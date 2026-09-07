#include "core/quadrature.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

// How often a position is recorded for the velocity fit. 64 samples is 1.33 ms
// at 48 kHz -- fine enough that a 10 ms window holds several marks, coarse
// enough that the fit costs nothing.
constexpr std::size_t kMarkInterval = 64;
constexpr std::size_t kHistoryDepth = 32;

// Carrier cycles the calibration averages over before it is trusted.
constexpr double kCalibrationTurns = 8.0;

// Coherent samples required before the tracker will call itself locked.
// 16 at 48 kHz is a third of a millisecond -- instant for a carrier, and long
// enough that noise essentially never manages it.
constexpr std::uint32_t kCoherentRun = 16;

double wrap_pi(double angle) {
    while (angle > kPi) angle -= kTwoPi;
    while (angle < -kPi) angle += kTwoPi;
    return angle;
}

}  // namespace

bool QuadratureTracker::configure(const QuadratureConfig& config) {
    if (config.carrier_hz <= 0.0) return false;
    // At or below twice the carrier there is no speed at all that can be
    // tracked, so this is a broken configuration rather than a slow one.
    if (config.sample_rate <= 2.0 * config.carrier_hz) return false;
    if (config.slew_limit_pi <= 0.0 || config.slew_limit_pi >= 1.0) return false;

    config_ = config;
    reset();
    return true;
}

double QuadratureTracker::max_speed_ratio() const {
    if (config_.carrier_hz <= 0.0) return 0.0;
    return config_.sample_rate / (2.0 * config_.carrier_hz);
}

void QuadratureTracker::reset() {
    turns_ = 0;
    phase_ = 0.0;
    have_phase_ = false;
    position_s_ = config_.origin_s;
    velocity_ = 0.0f;
    level_ = 0.0f;
    radius_ = 0.0;
    locked_ = false;
    coherent_run_ = 0;
    slew_events_ = 0;

    centre_[0] = centre_[1] = 0.0;
    gain_[0] = gain_[1] = 1.0;
    range_min_[0] = range_min_[1] = 0.0;
    range_max_[0] = range_max_[1] = 0.0;
    calibration_samples_ = 0;
    turned_since_calibration_ = 0.0;

    history_.assign(kHistoryDepth, Mark{});
    history_head_ = 0;
    history_count_ = 0;
    since_mark_ = 0;
    elapsed_s_ = 0.0;
}

void QuadratureTracker::update_calibration(double l, double r) {
    if (calibration_samples_ == 0) {
        range_min_[0] = range_max_[0] = l;
        range_min_[1] = range_max_[1] = r;
    } else {
        range_min_[0] = std::min(range_min_[0], l);
        range_max_[0] = std::max(range_max_[0], l);
        range_min_[1] = std::min(range_min_[1], r);
        range_max_[1] = std::max(range_max_[1], r);
    }
    ++calibration_samples_;

    // Only committed once the vector has genuinely gone round several times.
    // NET rotation, not distance travelled: jitter at a standstill accumulates
    // |step| without ever going anywhere, and a min/max taken while parked spans
    // a point rather than a circle -- committing that would set the centre to
    // wherever the record happened to stop and collapse one gain to nothing.
    // A stationary record must keep the calibration it already has.
    if (std::fabs(turned_since_calibration_) < kCalibrationTurns * kTwoPi) return;

    for (int c = 0; c < 2; ++c) {
        const double half = (range_max_[c] - range_min_[c]) * 0.5;
        if (half > 1e-6) {
            centre_[c] = (range_max_[c] + range_min_[c]) * 0.5;
            gain_[c] = half;
        }
    }
    // The gains just changed, so the radius is now on a different scale -- after
    // normalising it sits on the unit circle. Rescaling the reference with it is
    // what stops the commit from tripping the coherence gate; without this a
    // quiet signal (gain 1.0 -> 0.02, a fiftyfold jump in radius) unlocks itself
    // at the exact moment its calibration became good.
    radius_ = 1.0;
    calibration_samples_ = 0;
    turned_since_calibration_ = 0.0;
}

DecoderSample QuadratureTracker::submit(const float* interleaved, std::size_t frames,
                                        double now_s) {
    DecoderSample sample;
    sample.time_s = now_s;
    if (interleaved == nullptr || frames == 0 || config_.carrier_hz <= 0.0) {
        sample.position_s = -1.0;
        return sample;
    }

    const double dt = 1.0 / config_.sample_rate;
    const double slew_limit = config_.slew_limit_pi * kPi;
    double peak_radius = 0.0;

    for (std::size_t i = 0; i < frames; ++i) {
        const double raw_l = interleaved[i * 2];
        const double raw_r = interleaved[i * 2 + 1];
        update_calibration(raw_l, raw_r);

        const double x = (raw_l - centre_[0]) / (gain_[0] > 1e-9 ? gain_[0] : 1.0);
        const double y = (raw_r - centre_[1]) / (gain_[1] > 1e-9 ? gain_[1] : 1.0);

        const double radius = std::sqrt(x * x + y * y);
        peak_radius = std::max(peak_radius, radius);

        const double raw_peak = std::max(std::fabs(raw_l), std::fabs(raw_r));
        const bool loud_enough = raw_peak >= config_.silence_level;

        // Seeded on the first usable sample rather than crept up to from zero.
        // Starting at zero makes `radius < radius_ * 3` false forever, so the
        // tracker would never lock at all -- and the coherence gate would look
        // like a broken signal instead of a broken initialisation.
        if (loud_enough && radius_ <= 1e-9) radius_ = radius;
        // Tracked slowly afterwards, so the reference survives the 2:1 swing a
        // real control record's amplitude modulation puts on the radius.
        if (loud_enough) radius_ += (radius - radius_) * 0.001;

        // Coherence, not level. On noise the radius wanders; on a carrier it
        // sits near its reference whatever the volume. The band is wide because
        // a genuine control record swings the radius by 2:1 at the carrier rate.
        const bool coherent = radius_ > 1e-9 && radius > radius_ * 0.3 &&
                              radius < radius_ * 3.0;

        // A run of coherent samples, not just one. Noise crosses the band
        // constantly by chance, and a single passing sample was enough to let a
        // random walk carry the position away -- this project has already been
        // bitten once by treating an instantaneous coincidence as a signal
        // (audio_probe calling a room mic "TIMECODE"). A real carrier satisfies
        // this within a millisecond and never breaks it.
        if (loud_enough && coherent) {
            if (coherent_run_ < kCoherentRun) ++coherent_run_;
        } else {
            coherent_run_ = 0;
        }
        const bool present = coherent_run_ >= kCoherentRun;

        if (!present) {
            // A needle up, a stopped platter and an unplugged cable all look the
            // same here, and all three must freeze rather than invent motion.
            have_phase_ = false;
            locked_ = false;
            continue;
        }

        const double phase = std::atan2(y, x);
        if (!have_phase_) {
            phase_ = phase;
            have_phase_ = true;
            locked_ = true;
            continue;
        }

        const double step = wrap_pi(phase - phase_);
        if (std::fabs(step) > slew_limit) {
            // Past this the unwrap cannot tell forwards from backwards, and a
            // hard forward throw would read as a backspin. Holding loses the
            // over-speed movement; reversing would invent motion in the wrong
            // direction, which is far worse on a picture.
            ++slew_events_;
            locked_ = false;
            phase_ = phase;
            continue;
        }

        // The turn count, the only accumulated quantity, and an exact one.
        const double unwrapped = phase_ + step;
        if (unwrapped > kPi) {
            ++turns_;
        } else if (unwrapped < -kPi) {
            --turns_;
        }
        phase_ = wrap_pi(unwrapped);
        turned_since_calibration_ += step;  // signed: jitter cancels, rotation does not
        locked_ = true;

        position_s_ = config_.origin_s +
                      (static_cast<double>(turns_) + phase_ / kTwoPi) / config_.carrier_hz;

        elapsed_s_ += dt;
        if (++since_mark_ >= kMarkInterval) {
            since_mark_ = 0;
            history_[history_head_] = Mark{elapsed_s_, position_s_};
            history_head_ = (history_head_ + 1) % kHistoryDepth;
            if (history_count_ < kHistoryDepth) ++history_count_;
        }
    }

    // Velocity as a least-squares slope over the recent marks. Derived from the
    // positions rather than tracked separately, so it can never disagree with
    // the position it is supposed to describe.
    if (history_count_ >= 2) {
        double sum_t = 0.0, sum_p = 0.0, sum_tt = 0.0, sum_tp = 0.0;
        int used = 0;
        for (std::size_t k = 0; k < history_count_; ++k) {
            const Mark& mark = history_[(history_head_ + kHistoryDepth - 1 - k) %
                                        kHistoryDepth];
            if (elapsed_s_ - mark.t > config_.velocity_window_s && used >= 2) break;
            sum_t += mark.t;
            sum_p += mark.position;
            sum_tt += mark.t * mark.t;
            sum_tp += mark.t * mark.position;
            ++used;
        }
        const double n = used;
        const double denominator = n * sum_tt - sum_t * sum_t;
        if (used >= 2 && std::fabs(denominator) > 1e-12) {
            velocity_ = static_cast<float>((n * sum_tp - sum_t * sum_p) / denominator);
        }
    }
    if (!locked_) velocity_ = 0.0f;

    level_ = static_cast<float>(std::min(peak_radius * (gain_[0] > 1e-9 ? gain_[0] : 1.0),
                                         1.0));

    sample.pitch = velocity_;
    sample.signal_level = level_;
    sample.locked = locked_;
    sample.position_s = locked_ ? position_s_ : -1.0;
    return sample;
}

// --- the generator -----------------------------------------------------------

bool generate_quadrature(const QuadratureSignal& signal,
                         const std::function<double(double)>& position_at,
                         std::size_t frames, std::vector<float>& out) {
    if (signal.carrier_hz <= 0.0 || signal.sample_rate <= 0.0 || !position_at) {
        return false;
    }

    out.assign(frames * 2, 0.0f);
    std::uint32_t seed = signal.seed;
    const auto noise = [&seed, &signal]() {
        seed = seed * 1664525u + 1013904223u;
        return (static_cast<double>(seed >> 8) / 8388608.0 - 1.0) * signal.noise;
    };

    for (std::size_t i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / signal.sample_rate;
        // The angle is the position, evaluated. Never integrated: the rig obeys
        // the same rule the module does, so a drifting generator can never be
        // mistaken for a drifting tracker.
        const double angle = kTwoPi * signal.carrier_hz * position_at(t);

        const double x = signal.amplitude * std::cos(angle) + signal.dc_left + noise();
        const double y = signal.amplitude * signal.gain_right * std::sin(angle) +
                         signal.dc_right + noise();

        out[i * 2] = static_cast<float>(signal.swap_channels ? y : x);
        out[i * 2 + 1] = static_cast<float>(signal.swap_channels ? x : y);
    }
    return true;
}

}  // namespace svj
