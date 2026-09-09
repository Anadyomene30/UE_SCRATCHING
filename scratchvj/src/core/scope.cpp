#include "core/scope.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;

}  // namespace

LissajousScope::LissajousScope() { configure(ScopeConfig{}); }

bool LissajousScope::configure(const ScopeConfig& config) {
    if (config.window == 0 || config.trace == 0) return false;
    config_ = config;
    ring_.assign(config_.trace * 2, 0.0f);
    reset();
    return true;
}

void LissajousScope::reset() {
    reading_ = ScopeReading{};
    sum_x_ = sum_y_ = sum_xx_ = sum_yy_ = sum_xy_ = 0.0;
    count_ = 0;
    crossing_level_ = 0.0;
    last_above_ = false;
    have_last_ = false;
    crossings_ = 0;
    ring_head_ = 0;
    ring_count_ = 0;
}

void LissajousScope::submit(const float* interleaved, std::size_t frames) {
    if (interleaved == nullptr) return;

    for (std::size_t i = 0; i < frames; ++i) {
        const double x = static_cast<double>(interleaved[i * 2]);
        const double y = static_cast<double>(interleaved[i * 2 + 1]);

        sum_x_ += x;
        sum_y_ += y;
        sum_xx_ += x * x;
        sum_yy_ += y * y;
        sum_xy_ += x * y;

        const bool above = x > crossing_level_;
        if (have_last_ && above != last_above_) ++crossings_;
        last_above_ = above;
        have_last_ = true;

        ring_[ring_head_ * 2] = static_cast<float>(x);
        ring_[ring_head_ * 2 + 1] = static_cast<float>(y);
        ring_head_ = (ring_head_ + 1) % config_.trace;
        if (ring_count_ < config_.trace) ++ring_count_;

        if (++count_ >= config_.window) publish();
    }
}

void LissajousScope::publish() {
    const double n = static_cast<double>(count_);
    const double cx = sum_x_ / n;
    const double cy = sum_y_ / n;

    // Variance about the measured centre, so an offset is never mistaken for
    // amplitude. A sinusoid's peak is sqrt(2) times its RMS.
    const double var_x = std::max(0.0, sum_xx_ / n - cx * cx);
    const double var_y = std::max(0.0, sum_yy_ / n - cy * cy);
    const double ax = std::sqrt(2.0 * var_x);
    const double ay = std::sqrt(2.0 * var_y);

    ScopeReading out;
    out.centre_x = static_cast<float>(cx);
    out.centre_y = static_cast<float>(cy);
    out.amplitude_x = static_cast<float>(ax);
    out.amplitude_y = static_cast<float>(ay);
    // A crossing every half turn, hence the halving.
    out.turns = static_cast<float>(crossings_) * 0.5f;
    out.windows = reading_.windows + 1;

    const double floor_level = static_cast<double>(config_.silence_level);
    if (ax < floor_level && ay < floor_level) {
        out.verdict = ScopeVerdict::NoSignal;
    } else if (static_cast<double>(out.turns) < config_.min_turns) {
        out.verdict = ScopeVerdict::TooSlow;
    } else {
        out.verdict = ScopeVerdict::Measured;

        out.balance_db = static_cast<float>(20.0 * std::log10(ay / ax));

        // For x = A.cos(t) and y = B.sin(t + phi), the covariance of the two
        // legs normalised to unit amplitude is exactly sin(phi)/2. So the
        // phase error falls out of sums already being kept, with no fit and
        // no iteration -- and it is EXACT for a sinusoidal pair rather than
        // an approximation that degrades as the error grows.
        const double covariance = sum_xy_ / n - cx * cy;
        const double sin_phi = std::clamp(2.0 * covariance / (ax * ay), -1.0, 1.0);
        out.phase_error_deg = static_cast<float>(std::asin(sin_phi) * 180.0 / kPi);
    }

    reading_ = out;

    // The next window counts its crossings against the centre just measured.
    crossing_level_ = cx;
    sum_x_ = sum_y_ = sum_xx_ = sum_yy_ = sum_xy_ = 0.0;
    count_ = 0;
    crossings_ = 0;
    have_last_ = false;
}

void LissajousScope::trace(std::vector<float>& out) const {
    out.clear();
    out.reserve(ring_count_ * 2);
    // Oldest first: the ring is full from `ring_head_` onwards, and before it
    // has filled once, from zero.
    const std::size_t start = ring_count_ < config_.trace ? 0 : ring_head_;
    for (std::size_t i = 0; i < ring_count_; ++i) {
        const std::size_t index = (start + i) % config_.trace;
        out.push_back(ring_[index * 2]);
        out.push_back(ring_[index * 2 + 1]);
    }
}

}  // namespace svj
