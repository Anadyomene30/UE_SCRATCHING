#include "core/gestures.h"

#include <cmath>

namespace svj {

void GestureTracker::reset() {
    have_previous_ = false;
    velocity_ = 0.0f;
    acceleration_ = 0.0f;
    scratch_rate_ = 0.0f;
    backspin_ = false;
    scratching_ = false;
    holding_ = false;
    reversals_.clear();
    in_backspin_run_ = false;
}

void GestureTracker::prune(double t_s) {
    const double cutoff = t_s - config_.scratch_window_s;
    while (!reversals_.empty() && reversals_.front() < cutoff) reversals_.pop_front();
}

void GestureTracker::update(double t_s, float velocity, float confidence) {
    // Lost lock: hold everything as it was. Deliberately we do not decay towards
    // zero either, because that would still move the picture on a dropout.
    if (confidence < config_.min_confidence) {
        holding_ = true;
        return;
    }
    holding_ = false;

    if (!have_previous_) {
        have_previous_ = true;
        previous_t_ = t_s;
        previous_velocity_ = velocity;
        velocity_ = velocity;
        acceleration_ = 0.0f;
        return;
    }

    const double dt = t_s - previous_t_;
    if (dt <= 0.0) {
        velocity_ = velocity;
        return;
    }

    // A reversal is a sign change through zero. Exact zeros are not reversals on
    // their own, otherwise a platter resting at a standstill would chatter.
    const bool crossed = (previous_velocity_ > 0.0f && velocity < 0.0f) ||
                         (previous_velocity_ < 0.0f && velocity > 0.0f);
    if (crossed) reversals_.push_back(t_s);
    prune(t_s);
    scratch_rate_ = static_cast<float>(reversals_.size()) / config_.scratch_window_s;

    const float raw_accel = static_cast<float>((velocity - previous_velocity_) / dt);
    const float tau = config_.accel_smoothing_ms * 0.001f;
    if (tau > 1e-6f) {
        const float alpha = 1.0f - std::exp(-static_cast<float>(dt) / tau);
        acceleration_ += (raw_accel - acceleration_) * alpha;
    } else {
        acceleration_ = raw_accel;
    }

    if (velocity <= config_.backspin_velocity) {
        if (!in_backspin_run_) {
            in_backspin_run_ = true;
            backspin_since_ = t_s;
        }
        backspin_ = (t_s - backspin_since_) >= config_.backspin_hold_s;
    } else {
        in_backspin_run_ = false;
        backspin_ = false;
    }

    scratching_ = std::fabs(velocity - 1.0f) > config_.scratch_deviation;

    velocity_ = velocity;
    previous_velocity_ = velocity;
    previous_t_ = t_s;
}

}  // namespace svj
