#include "core/timecode.h"

#include <algorithm>
#include <cmath>

namespace svj {

void TimecodeTracker::reset() {
    state_ = TimecodeState{};
    have_previous_ = false;
    ramping_ = false;
    ramp_remaining_ = 0.0;
    ramp_offset_ = 0.0;
    jump_count_ = 0;
}

void TimecodeTracker::set_mode(TransportMode mode) {
    if (mode == config_.mode) return;
    config_.mode = mode;
    // Changing mode must not move the playhead: absolute mode picks the position
    // back up from wherever the output already sits.
    ramping_ = false;
    ramp_offset_ = state_.position_s - previous_raw_position_;
}

void TimecodeTracker::hold() {
    state_.velocity = 0.0f;
    state_.jumped = false;
}

void TimecodeTracker::advance_output(double dt, float velocity) {
    state_.position_s += static_cast<double>(velocity) * dt;
}

const TimecodeState& TimecodeTracker::submit(const DecoderSample& sample) {
    state_.jumped = false;

    const double dt = have_previous_ ? sample.time_s - previous_time_ : 0.0;
    previous_time_ = sample.time_s;

    // ---- Internal mode: the platter is not consulted at all. ----
    if (config_.mode == TransportMode::Internal) {
        state_.link = LinkState::Ok;
        state_.confidence = 1.0f;
        state_.platter_stopped = false;
        state_.velocity = 1.0f;
        if (have_previous_ && dt > 0.0) advance_output(dt, 1.0f);
        have_previous_ = true;
        return state_;
    }

    // ---- No carrier. The whole point of the profile split. ----
    if (sample.signal_level < config_.silence_level) {
        if (config_.profile == SignalProfile::Wireless) {
            // The dock emits even at a standstill, so silence is a lost link.
            // Freeze rather than decay: decaying towards zero would still move
            // the picture, which is the thing a dropout must never do.
            state_.link = LinkState::Lost;
            state_.confidence = 0.0f;
            state_.platter_stopped = false;
        } else {
            // A needle in a groove that is not turning. Entirely normal.
            state_.link = LinkState::Ok;
            state_.confidence = 1.0f;
            state_.platter_stopped = true;
        }
        hold();
        have_previous_ = true;
        return state_;
    }

    // ---- Carrier present but the bits are unreadable. ----
    // Coast on pitch: the decoder can still track the waveform's speed when it
    // cannot resolve the absolute position.
    if (sample.position_s < 0.0) {
        state_.link = LinkState::Degraded;
        state_.confidence = 0.35f;
        state_.velocity = sample.pitch;
        state_.platter_stopped = std::fabs(sample.pitch) < config_.stopped_pitch;
        if (have_previous_ && dt > 0.0) advance_output(dt, sample.pitch);
        have_previous_ = true;
        return state_;
    }

    // ---- Locked. ----
    state_.link = LinkState::Ok;
    state_.confidence = 1.0f;
    state_.velocity = sample.pitch;
    state_.platter_stopped = std::fabs(sample.pitch) < config_.stopped_pitch;

    if (!have_previous_) {
        previous_raw_position_ = sample.position_s;
        ramp_offset_ = 0.0;
        state_.position_s = sample.position_s;
        have_previous_ = true;
        return state_;
    }

    // A step larger than playback can account for is a discontinuity: a needle
    // drop, or a Phase remote lifted and put back down.
    const double expected = previous_raw_position_ + static_cast<double>(sample.pitch) * dt;
    const double delta = sample.position_s - expected;
    const bool jumped = std::fabs(delta) > config_.jump_threshold_s;

    if (jumped) {
        ++jump_count_;
        state_.jumped = true;
    }

    switch (config_.mode) {
        case TransportMode::Absolute:
            if (jumped && config_.relock_ramp_s > 0.0) {
                // Glide from where the output currently is to the new position,
                // so replacing a remote does not crack the picture.
                ramp_offset_ = state_.position_s - sample.position_s;
                ramp_remaining_ = config_.relock_ramp_s;
                ramping_ = true;
            }
            if (ramping_) {
                ramp_remaining_ -= dt;
                if (ramp_remaining_ <= 0.0) {
                    ramping_ = false;
                    ramp_offset_ = 0.0;
                } else {
                    const double fraction = ramp_remaining_ / config_.relock_ramp_s;
                    ramp_offset_ *= fraction;
                }
            }
            state_.position_s = sample.position_s + ramp_offset_;
            break;

        case TransportMode::Relative:
            // Movement counts, absolute placement does not. A discontinuity is
            // absorbed by re-baselining, which is precisely why relative mode is
            // what you actually use in a set -- and it matters more with a Phase
            // than with vinyl, since repositioning means lifting the remote.
            if (jumped) {
                ramp_offset_ = state_.position_s - sample.position_s;
            } else {
                advance_output(sample.position_s - previous_raw_position_, 1.0f);
                ramp_offset_ = state_.position_s - sample.position_s;
            }
            state_.position_s = sample.position_s + ramp_offset_;
            break;

        case TransportMode::Internal:
            break;  // handled above
    }

    previous_raw_position_ = sample.position_s;
    return state_;
}

}  // namespace svj
