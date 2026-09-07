#include "dvs/decoder.h"

#include <algorithm>
#include <cmath>

extern "C" {
#include "timecoder.h"
}

namespace svj::dvs {

struct TimecodeDecoder::Impl {
    timecoder tc{};
};

std::vector<std::string> known_definitions() {
    // xwax's own table, in its own order. Listed rather than discovered because
    // its `timecodes` array is not exported.
    return {"serato_2a",   "serato_2b",      "serato_cd", "traktor_a", "traktor_b",
            "mixvibes_v2", "mixvibes_7inch", "pioneer_a", "pioneer_b"};
}

TimecodeDecoder::TimecodeDecoder() = default;

TimecodeDecoder::~TimecodeDecoder() { close(); }

bool TimecodeDecoder::open(const std::string& definition, unsigned sample_rate,
                           bool phono) {
    close();
    if (sample_rate < 8000 || sample_rate > 192000) return false;

    timecode_def* def = timecoder_find_definition(definition.c_str());
    if (def == nullptr) return false;

    impl_ = new Impl();
    // Speed 1.0: the record turns at its nominal rate. The Phase's remotes are
    // configured for 33 RPM in Phase Manager, which IS the nominal rate for the
    // Serato definitions, so no correction belongs here.
    timecoder_init(&impl_->tc, def, 1.0, sample_rate, phono);
    definition_ = definition;
    sample_rate_ = sample_rate;
    return true;
}

void TimecodeDecoder::close() {
    if (impl_ != nullptr) {
        timecoder_clear(&impl_->tc);
        delete impl_;
        impl_ = nullptr;
    }
    definition_.clear();
    sample_rate_ = 0;
    level_ = 0.0f;
}

DecoderSample TimecodeDecoder::submit(const std::int16_t* interleaved,
                                      std::size_t frames, double now_s) {
    DecoderSample sample;
    sample.time_s = now_s;
    if (impl_ == nullptr || interleaved == nullptr || frames == 0) return sample;

    std::int16_t peak = 0;
    for (std::size_t i = 0; i < frames * 2; ++i) {
        const std::int16_t value = interleaved[i];
        // -32768 negated overflows; clamped first so the peak stays honest.
        const std::int16_t magnitude =
            value == INT16_MIN ? INT16_MAX : static_cast<std::int16_t>(std::abs(value));
        peak = std::max(peak, magnitude);
    }
    level_ = static_cast<float>(peak) / 32767.0f;

    timecoder_submit(&impl_->tc, const_cast<std::int16_t*>(interleaved), frames);

    // Exactly xwax's own sync_to_timecode: the bitstream gives an absolute
    // position, `when` says how long ago that reading was, and the pitch
    // extrapolates it forward. Doing it any other way -- accumulating pitch,
    // say -- would be the integrator design principle 1 forbids, and would
    // drift over a set.
    double when = 0.0;
    const signed int timecode = timecoder_get_position(&impl_->tc, &when);
    sample.pitch = static_cast<float>(timecoder_get_pitch(&impl_->tc));
    sample.signal_level = level_;

    if (timecode < 0) {
        // No lock. core/timecode reads a negative position as exactly that, and
        // decides what to do about it -- freezing rather than teleporting.
        sample.position_s = -1.0;
        return sample;
    }

    const double resolution = timecoder_get_resolution(&impl_->tc);
    if (resolution <= 0.0) {
        sample.position_s = -1.0;
        return sample;
    }
    sample.position_s =
        static_cast<double>(timecode) / resolution + sample.pitch * when;
    return sample;
}

DecoderSample TimecodeDecoder::submit(const float* interleaved, std::size_t frames,
                                      double now_s) {
    DecoderSample sample;
    sample.time_s = now_s;
    if (impl_ == nullptr || interleaved == nullptr || frames == 0) return sample;

    scratch_.resize(frames * 2);
    for (std::size_t i = 0; i < frames * 2; ++i) {
        // Clipped, not wrapped. A wrapped sample flips sign at full scale, which
        // the phase detector reads as a violent direction reversal -- a fault
        // that would look like the record being scratched when it is not.
        const float clamped = std::clamp(interleaved[i], -1.0f, 1.0f);
        scratch_[i] = static_cast<std::int16_t>(clamped * 32767.0f);
    }
    return submit(scratch_.data(), frames, now_s);
}

}  // namespace svj::dvs
