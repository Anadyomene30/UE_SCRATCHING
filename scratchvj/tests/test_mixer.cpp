#include <cmath>

#include "core/mixer.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("crossfader: every curve is fully on one deck at each end") {
    for (const FaderCurve curve : {FaderCurve::Smooth, FaderCurve::Linear,
                                   FaderCurve::Sharp, FaderCurve::Cut}) {
        const MixWeights left = crossfader_weights(0.0f, curve);
        CHECK_NEAR(left.a, 1.0, 1e-5);
        CHECK_NEAR(left.b, 0.0, 1e-5);

        const MixWeights right = crossfader_weights(1.0f, curve);
        CHECK_NEAR(right.a, 0.0, 1e-5);
        CHECK_NEAR(right.b, 1.0, 1e-5);
    }
}

SVJ_TEST("crossfader: A SHARP CURVE REACHES FULL WITHIN A FEW PERCENT") {
    // This is what makes a transform or a crab playable. On a curve that fades
    // instead, the gesture simply does not exist.
    const MixWeights just_off = crossfader_weights(0.03f, FaderCurve::Sharp);
    CHECK_NEAR(just_off.b, 1.0, 1e-5);
    CHECK_NEAR(just_off.a, 1.0, 1e-5);  // and A has not started closing yet

    // The same nudge on a smooth curve is barely a whisper.
    const MixWeights smooth = crossfader_weights(0.03f, FaderCurve::Smooth);
    CHECK(smooth.b < 0.1f);
}

SVJ_TEST("crossfader: the sharp curve leaves both decks open across the middle") {
    for (float x = 0.1f; x <= 0.9f; x += 0.1f) {
        const MixWeights w = crossfader_weights(x, FaderCurve::Sharp);
        CHECK_NEAR(w.a, 1.0, 1e-5);
        CHECK_NEAR(w.b, 1.0, 1e-5);
    }
}

SVJ_TEST("crossfader: linear sums to one, smooth keeps constant power") {
    for (float x = 0.0f; x <= 1.0f; x += 0.05f) {
        const MixWeights linear = crossfader_weights(x, FaderCurve::Linear);
        CHECK_NEAR(linear.a + linear.b, 1.0, 1e-5);

        const MixWeights smooth = crossfader_weights(x, FaderCurve::Smooth);
        CHECK_NEAR(smooth.a * smooth.a + smooth.b * smooth.b, 1.0, 1e-5);
    }
}

SVJ_TEST("crossfader: a cut curve switches at the middle with nothing in between") {
    CHECK_NEAR(crossfader_weights(0.49f, FaderCurve::Cut).a, 1.0, 1e-6);
    CHECK_NEAR(crossfader_weights(0.49f, FaderCurve::Cut).b, 0.0, 1e-6);
    CHECK_NEAR(crossfader_weights(0.51f, FaderCurve::Cut).b, 1.0, 1e-6);
}

SVJ_TEST("crossfader: a position outside the travel is clamped") {
    CHECK_NEAR(crossfader_weights(-3.0f, FaderCurve::Linear).a, 1.0, 1e-6);
    CHECK_NEAR(crossfader_weights(4.0f, FaderCurve::Linear).b, 1.0, 1e-6);
}

SVJ_TEST("mixer: the channel faders scale what the crossfader lets through") {
    const MixWeights w = mix_weights(0.5f, 0.5f, 1.0f, FaderCurve::Linear);
    CHECK_NEAR(w.a, 0.25, 1e-6);
    CHECK_NEAR(w.b, 0.5, 1e-6);

    const MixWeights closed = mix_weights(0.5f, 0.0f, 0.0f, FaderCurve::Sharp);
    CHECK_NEAR(closed.a, 0.0, 1e-6);
    CHECK_NEAR(closed.b, 0.0, 1e-6);
}

SVJ_TEST("mixer: out-of-range fader values are clamped rather than amplifying") {
    const MixWeights w = mix_weights(0.0f, 5.0f, -2.0f, FaderCurve::Linear);
    CHECK_NEAR(w.a, 1.0, 1e-6);
    CHECK_NEAR(w.b, 0.0, 1e-6);
}

SVJ_TEST("cuts: a steady fader produces no cuts") {
    CutDetector detector;
    for (int i = 0; i < 100; ++i) detector.update(i * 0.01, 0.9f);
    CHECK_NEAR(detector.cuts_per_second(), 0.0, 1e-6);
    CHECK(!detector.transforming());
}

SVJ_TEST("cuts: a transform is counted and recognised") {
    CutDetector detector;
    double t = 0.0;
    for (int i = 0; i < 16; ++i) {
        t += 0.0625;  // eight cuts per second
        detector.update(t, i % 2 == 0 ? 0.02f : 0.98f);
    }
    CHECK(detector.cuts_per_second() >= 7.0f);
    CHECK(detector.transforming());
}

SVJ_TEST("cuts: A FADER RESTING NEAR THE MIDDLE DOES NOT CHATTER") {
    // Without hysteresis, a hand holding the fader at the midpoint would invent a
    // stream of cuts and drive every mapped effect from noise.
    CutDetector detector;
    double t = 0.0;
    for (int i = 0; i < 200; ++i) {
        t += 0.005;
        detector.update(t, 0.5f + (i % 2 == 0 ? 0.005f : -0.005f));
    }
    CHECK_NEAR(detector.cuts_per_second(), 0.0, 1e-6);
}

SVJ_TEST("cuts: the rate decays once the cutting stops") {
    CutDetector detector(1.0f);
    double t = 0.0;
    for (int i = 0; i < 10; ++i) {
        t += 0.05;
        detector.update(t, i % 2 == 0 ? 0.0f : 1.0f);
    }
    CHECK(detector.cuts_per_second() > 4.0f);

    for (int i = 0; i < 40; ++i) {
        t += 0.05;
        detector.update(t, 1.0f);
    }
    CHECK_NEAR(detector.cuts_per_second(), 0.0, 1e-6);
}

SVJ_TEST("cuts: resetting clears the history") {
    CutDetector detector;
    double t = 0.0;
    for (int i = 0; i < 10; ++i) detector.update(t += 0.05, i % 2 == 0 ? 0.0f : 1.0f);
    detector.reset();
    CHECK_NEAR(detector.cuts_per_second(), 0.0, 1e-6);
}

// --- the third layer ---------------------------------------------------------

SVJ_TEST("stack: THE CROSSFADER DOES NOT REACH THE OVERLAY") {
    // The property that makes a third layer worth having. A logo or a mask that
    // dipped on every transition would be worse than not having one at all.
    Layer overlay;
    overlay.enabled = true;
    overlay.opacity = 0.8f;

    for (float x = 0.0f; x <= 1.0f; x += 0.1f) {
        const StackWeights stack = stack_weights(x, 1.0f, 1.0f, FaderCurve::Sharp, overlay);
        CHECK_NEAR(stack.overlay, 0.8, 1e-5);
    }
}

SVJ_TEST("stack: the two decks below still mix exactly as they did alone") {
    Layer overlay;
    overlay.enabled = true;

    for (float x = 0.0f; x <= 1.0f; x += 0.1f) {
        const MixWeights pair = mix_weights(x, 0.7f, 0.4f, FaderCurve::Smooth);
        const StackWeights stack =
            stack_weights(x, 0.7f, 0.4f, FaderCurve::Smooth, overlay);
        CHECK_NEAR(stack.a, pair.a, 1e-6);
        CHECK_NEAR(stack.b, pair.b, 1e-6);
    }
}

SVJ_TEST("stack: a disabled overlay contributes nothing, whatever its opacity") {
    Layer overlay;
    overlay.enabled = false;
    overlay.opacity = 1.0f;
    CHECK_NEAR(stack_weights(0.5f, 1.0f, 1.0f, FaderCurve::Linear, overlay).overlay,
               0.0, 1e-6);
}

SVJ_TEST("stack: an out of range opacity is clamped rather than trusted") {
    // Opacity arrives from a mapping, and a mapping can be scaled to anything.
    Layer overlay;
    overlay.enabled = true;
    overlay.opacity = 4.0f;
    CHECK_NEAR(stack_weights(0.5f, 1.0f, 1.0f, FaderCurve::Linear, overlay).overlay,
               1.0, 1e-6);
    overlay.opacity = -2.0f;
    CHECK_NEAR(stack_weights(0.5f, 1.0f, 1.0f, FaderCurve::Linear, overlay).overlay,
               0.0, 1e-6);
}
