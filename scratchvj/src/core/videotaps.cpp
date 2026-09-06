#include "core/videotaps.h"

#include <algorithm>
#include <cmath>

namespace svj {

bool is_multi_tap_effect(EffectType type) {
    return type == EffectType::Delay || type == EffectType::SlitScan;
}

TapPlan plan_taps(const EffectUnit& unit, double played_s, double velocity,
                  double beat_duration_s, double clip_duration_s, ClipPlayMode mode) {
    TapPlan plan;
    if (!is_multi_tap_effect(unit.type) || clip_duration_s <= 0.0) {
        plan.count = 1;
        plan.position_s[0] = played_s;
        plan.weight[0] = 1.0f;
        plan.collapsed = true;
        return plan;
    }

    const EffectParams params = unit.video_params();

    // The spacing between moments, in WALL seconds. A synced delay takes it from
    // the grid; a slit scan spreads over a shorter span, because a scan whose
    // bands are a beat apart is a slideshow rather than a smear.
    const double spacing_s =
        unit.type == EffectType::Delay
            ? unit.sync.seconds(beat_duration_s, params.time)
            : 0.02 + params.time * 0.28;

    // The clip's own travel over that span. This is the whole conversion: a tap
    // is where the picture WAS, and where it was depends on how fast it is
    // going, not on how the clip happens to be numbered.
    const double travel = spacing_s * velocity;

    plan.count = kTapCount;
    plan.collapsed = std::fabs(travel) < 1e-9;

    double total = 0.0;
    for (int k = 0; k < kTapCount; ++k) {
        const double timeline = played_s - k * travel;
        plan.position_s[k] = fold_position(timeline, clip_duration_s, mode).position_s;

        // Feedback is what each repeat keeps of the one before, which is exactly
        // its meaning on the audio side. A slit scan weights its bands equally;
        // it picks one per column rather than summing them.
        const double weight = unit.type == EffectType::Delay
                                  ? std::pow(static_cast<double>(params.feedback), k)
                                  : 1.0;
        plan.weight[k] = static_cast<float>(weight);
        total += weight;
    }

    // Normalised, so turning feedback up lengthens the trail instead of
    // brightening the picture -- the fault that makes a naive trail clip to
    // white the moment the knob passes halfway.
    if (total > 1e-9) {
        for (int k = 0; k < kTapCount; ++k) {
            plan.weight[k] = static_cast<float>(plan.weight[k] / total);
        }
    }
    return plan;
}

}  // namespace svj
