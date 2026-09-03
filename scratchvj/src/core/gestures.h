// scratchvj — derived quantities from platter motion.
//
// Computed once here rather than in each consumer, so the video engine, the effect
// racks, the UI and Unreal all see the same numbers. These are what make visuals
// respond to the GESTURE rather than merely to the sound.
#pragma once

#include <cstddef>
#include <deque>

namespace svj {

struct GestureConfig {
    // Window over which direction reversals are counted.
    float scratch_window_s = 0.5f;

    // Sustained speed and duration that together constitute a backspin.
    float backspin_velocity = -2.5f;
    float backspin_hold_s = 0.2f;

    // How far from nominal speed the platter must be to count as "being worked".
    float scratch_deviation = 0.15f;

    float accel_smoothing_ms = 30.0f;

    // Below this timecode confidence the tracker HOLDS its last output instead of
    // reacting. A Phase dropout must freeze the picture, never teleport it.
    float min_confidence = 0.25f;
};

class GestureTracker {
public:
    explicit GestureTracker(GestureConfig config = {}) : config_(config) {}

    // `t_s` is a monotonic clock in seconds; `velocity` is the signed speed ratio
    // (1.0 nominal, negative backwards); `confidence` is the timecode lock quality.
    void update(double t_s, float velocity, float confidence);

    void reset();

    float velocity() const { return velocity_; }
    float acceleration() const { return acceleration_; }

    // Direction reversals per second over the configured window. The single best
    // driver of "how hard is this being scratched right now".
    float scratch_rate() const { return scratch_rate_; }

    bool backspin() const { return backspin_; }
    bool scratching() const { return scratching_; }

    // True while the tracker is coasting on stale data because confidence dropped.
    bool holding() const { return holding_; }

    const GestureConfig& config() const { return config_; }

private:
    void prune(double t_s);

    GestureConfig config_;

    bool have_previous_ = false;
    double previous_t_ = 0.0;
    float previous_velocity_ = 0.0f;

    float velocity_ = 0.0f;
    float acceleration_ = 0.0f;
    float scratch_rate_ = 0.0f;
    bool backspin_ = false;
    bool scratching_ = false;
    bool holding_ = false;

    std::deque<double> reversals_;
    double backspin_since_ = 0.0;
    bool in_backspin_run_ = false;
};

}  // namespace svj
