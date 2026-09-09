#include <cmath>

#include "core/videotaps.h"
#include "harness.h"

using namespace svj;

namespace {

EffectUnit delay_unit(float feedback, double beats) {
    EffectUnit unit;
    unit.type = EffectType::Delay;
    unit.enabled = true;
    unit.shared.mix = 1.0f;
    unit.shared.feedback = feedback;
    unit.sync.tempo = true;
    unit.sync.beats = beats;
    return unit;
}

}  // namespace

SVJ_TEST("taps: the first tap is always the frame on screen") {
    // Whatever else the plan does, tap zero has to be the dry picture, or the
    // effect's mix knob blends towards something that was never shown.
    const EffectUnit unit = delay_unit(0.6f, 0.5);
    const TapPlan plan = plan_taps(unit, 42.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
    CHECK_NEAR(plan.position_s[0], 42.0, 1e-9);
}

SVJ_TEST("taps: A TAP IS WHERE THE PICTURE WAS, WHICH DEPENDS ON HOW FAST IT MOVES") {
    // The claim the whole file rests on. At nominal speed the taps sit one
    // delay apart, exactly like the audio delay they are paired with. At double
    // speed the clip really travelled twice as far in that time, so they sit
    // twice as far apart -- the spread is derived, not tuned.
    const EffectUnit unit = delay_unit(0.5f, 1.0);
    const double beat = 0.5;  // 120 bpm

    const TapPlan nominal = plan_taps(unit, 100.0, 1.0, beat, 300.0, ClipPlayMode::Loop);
    CHECK_NEAR(nominal.position_s[1], 100.0 - beat, 1e-9);
    CHECK_NEAR(nominal.position_s[2], 100.0 - 2 * beat, 1e-9);

    const TapPlan fast = plan_taps(unit, 100.0, 2.0, beat, 300.0, ClipPlayMode::Loop);
    CHECK_NEAR(fast.position_s[1], 100.0 - 2 * beat, 1e-9);
}

SVJ_TEST("taps: SCRATCHING BACKWARDS PUTS THE TRAIL ON THE OTHER SIDE") {
    // What a feedback buffer cannot do, and the reason this is computed rather
    // than accumulated. The trail follows the MOTION: reverse the hand and the
    // offsets change sign on their own.
    const EffectUnit unit = delay_unit(0.5f, 1.0);
    const TapPlan forward = plan_taps(unit, 100.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
    const TapPlan backward = plan_taps(unit, 100.0, -1.0, 0.5, 300.0, ClipPlayMode::Loop);

    CHECK(forward.position_s[1] < 100.0);
    CHECK(backward.position_s[1] > 100.0);
    CHECK_NEAR(forward.position_s[1] - 100.0, -(backward.position_s[1] - 100.0), 1e-9);
}

SVJ_TEST("taps: a stopped record has no trail, and says so") {
    // The honest consequence of refusing to remember: an audio delay rings out
    // when the record stops because it has a buffer; these trails do not,
    // because a buffer is the integrator this design forbids. The plan reports
    // the collapse rather than leaving a caller to discover it.
    const EffectUnit unit = delay_unit(0.7f, 0.5);
    const TapPlan plan = plan_taps(unit, 33.0, 0.0, 0.5, 300.0, ClipPlayMode::Loop);
    CHECK(plan.collapsed);
    for (int k = 0; k < kTapCount; ++k) CHECK_NEAR(plan.position_s[k], 33.0, 1e-9);
}

SVJ_TEST("taps: the weights sum to one, so feedback lengthens instead of brightening") {
    // The fault this guards against is the classic naive trail: turn feedback
    // up and the picture clips to white instead of the tail growing.
    for (const float feedback : {0.0f, 0.4f, 0.9f}) {
        const TapPlan plan = plan_taps(delay_unit(feedback, 0.5), 10.0, 1.0, 0.5, 300.0,
                                       ClipPlayMode::Loop);
        double total = 0.0;
        for (int k = 0; k < kTapCount; ++k) total += plan.weight[k];
        CHECK_NEAR(total, 1.0, 1e-6);
    }
}

SVJ_TEST("taps: more feedback moves weight into the later repeats") {
    // Feedback means what it means on the audio side: how much each repeat
    // keeps of the one before.
    const TapPlan dry = plan_taps(delay_unit(0.1f, 0.5), 10.0, 1.0, 0.5, 300.0,
                                  ClipPlayMode::Loop);
    const TapPlan wet = plan_taps(delay_unit(0.9f, 0.5), 10.0, 1.0, 0.5, 300.0,
                                  ClipPlayMode::Loop);
    CHECK(wet.weight[4] > dry.weight[4]);
    CHECK(wet.weight[0] < dry.weight[0]);
}

SVJ_TEST("taps: a trail running off a loop comes back from the tail") {
    // Clamping instead would stick the whole trail on frame zero every time the
    // playhead neared the top of a short loop -- very visible on the grain loops
    // this instrument is full of.
    const EffectUnit unit = delay_unit(0.5f, 1.0);
    const TapPlan plan = plan_taps(unit, 0.2, 1.0, 0.5, 4.0, ClipPlayMode::Loop);
    CHECK(plan.position_s[1] > 3.0);  // wrapped to near the end
    CHECK(plan.position_s[1] < 4.0);
}

SVJ_TEST("taps: a single-frame effect asks for one tap, not eight") {
    // Eight uploads to draw one frame is a real cost on a real deck.
    EffectUnit unit;
    unit.type = EffectType::LowPass;
    unit.enabled = true;
    const TapPlan plan = plan_taps(unit, 5.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
    CHECK_EQ(plan.count, 1);
    CHECK(plan.collapsed);

    CHECK(is_multi_tap_effect(EffectType::Delay));
    CHECK(is_multi_tap_effect(EffectType::SlitScan));
    CHECK(!is_multi_tap_effect(EffectType::LowPass));
}

SVJ_TEST("taps: a slit scan spreads over a shorter span than a synced delay") {
    // A scan whose bands are a beat apart is a slideshow, not a smear. The two
    // effects share the machinery and not the timing.
    EffectUnit scan;
    scan.type = EffectType::SlitScan;
    scan.enabled = true;
    scan.shared.mix = 1.0f;
    scan.shared.time = 1.0f;

    const TapPlan scan_plan = plan_taps(scan, 50.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
    const TapPlan delay_plan =
        plan_taps(delay_unit(0.5f, 1.0), 50.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);

    const double scan_spread = 50.0 - scan_plan.position_s[kTapCount - 1];
    const double delay_spread = 50.0 - delay_plan.position_s[kTapCount - 1];
    CHECK(scan_spread < delay_spread);
    CHECK(scan_spread > 0.0);
}
