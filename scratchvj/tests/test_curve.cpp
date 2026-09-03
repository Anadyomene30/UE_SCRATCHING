#include "core/curve.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("curve: linear passes the input through unchanged") {
    Transform t;
    CHECK_NEAR(transform_apply(t, 0.0f), 0.0, 1e-6);
    CHECK_NEAR(transform_apply(t, 0.5f), 0.5, 1e-6);
    CHECK_NEAR(transform_apply(t, 1.0f), 1.0, 1e-6);
}

SVJ_TEST("curve: input outside the declared range is clamped") {
    Transform t;
    CHECK_NEAR(transform_apply(t, -3.0f), 0.0, 1e-6);
    CHECK_NEAR(transform_apply(t, 7.0f), 1.0, 1e-6);
}

SVJ_TEST("curve: output range is remapped, including descending ranges") {
    Transform t;
    t.out_lo = -180.0f;
    t.out_hi = 180.0f;
    CHECK_NEAR(transform_apply(t, 0.5f), 0.0, 1e-4);
    CHECK_NEAR(transform_apply(t, 1.0f), 180.0, 1e-4);

    Transform descending;
    descending.out_lo = 1.0f;
    descending.out_hi = 0.0f;
    CHECK_NEAR(transform_apply(descending, 0.25f), 0.75, 1e-6);
}

SVJ_TEST("curve: a degenerate input range yields the low output rather than a NaN") {
    Transform t;
    t.in_lo = 0.5f;
    t.in_hi = 0.5f;
    t.out_lo = 0.25f;
    CHECK_NEAR(transform_apply(t, 0.5f), 0.25, 1e-6);
}

SVJ_TEST("curve: invert mirrors the response") {
    Transform t;
    t.invert = true;
    CHECK_NEAR(transform_apply(t, 0.0f), 1.0, 1e-6);
    CHECK_NEAR(transform_apply(t, 0.25f), 0.75, 1e-6);
}

SVJ_TEST("curve: deadzone snaps the centre but still reaches both ends") {
    Transform t;
    t.deadzone = 0.2f;  // +/- 10% around the midpoint
    CHECK_NEAR(transform_apply(t, 0.5f), 0.5, 1e-6);
    CHECK_NEAR(transform_apply(t, 0.55f), 0.5, 1e-6);
    CHECK_NEAR(transform_apply(t, 0.45f), 0.5, 1e-6);

    // The rescale is what keeps a detented knob able to reach its extremes.
    CHECK_NEAR(transform_apply(t, 0.0f), 0.0, 1e-6);
    CHECK_NEAR(transform_apply(t, 1.0f), 1.0, 1e-6);
    CHECK(transform_apply(t, 0.7f) > 0.5f);
}

SVJ_TEST("curve: exponential and logarithmic bend in opposite directions") {
    Transform exponential;
    exponential.curve = CurveKind::Exponential;
    exponential.shape = 2.0f;
    CHECK_NEAR(transform_apply(exponential, 0.5f), 0.25, 1e-5);

    Transform logarithmic;
    logarithmic.curve = CurveKind::Logarithmic;
    logarithmic.shape = 2.0f;
    CHECK_NEAR(transform_apply(logarithmic, 0.25f), 0.5, 1e-5);
}

SVJ_TEST("curve: the S curve is symmetric and passes through its centre") {
    CHECK_NEAR(curve_eval(CurveKind::SCurve, 3.0f, 0.5f), 0.5, 1e-6);
    const double low = curve_eval(CurveKind::SCurve, 3.0f, 0.25f);
    const double high = curve_eval(CurveKind::SCurve, 3.0f, 0.75f);
    CHECK_NEAR(low + high, 1.0, 1e-6);
    CHECK(low < 0.25);  // eased at the ends
}

SVJ_TEST("curve: shape 1 makes every curve linear") {
    for (const CurveKind kind : {CurveKind::Exponential, CurveKind::Logarithmic, CurveKind::SCurve}) {
        CHECK_NEAR(curve_eval(kind, 1.0f, 0.3f), 0.3, 1e-6);
    }
}

SVJ_TEST("curve: a non-positive shape is treated as linear instead of exploding") {
    CHECK_NEAR(curve_eval(CurveKind::Exponential, 0.0f, 0.4f), 0.4, 1e-6);
    CHECK_NEAR(curve_eval(CurveKind::Exponential, -5.0f, 0.4f), 0.4, 1e-6);
}

SVJ_TEST("curve: smoothing primes on its first call rather than sweeping from zero") {
    Transform t;
    t.smoothing_ms = 100.0f;
    TransformState state;
    // A mapping touched for the first time must land on its value immediately.
    CHECK_NEAR(transform_apply_smoothed(t, 0.8f, 0.01f, state), 0.8, 1e-6);
}

SVJ_TEST("curve: smoothing then approaches a new target without overshooting") {
    Transform t;
    t.smoothing_ms = 50.0f;
    TransformState state;
    transform_apply_smoothed(t, 0.0f, 0.01f, state);

    float previous = 0.0f;
    for (int i = 0; i < 200; ++i) {
        const float value = transform_apply_smoothed(t, 1.0f, 0.01f, state);
        CHECK(value >= previous - 1e-6f);
        CHECK(value <= 1.0f + 1e-6f);
        previous = value;
    }
    CHECK_NEAR(previous, 1.0, 1e-3);
}

SVJ_TEST("curve: zero smoothing time bypasses the filter entirely") {
    Transform t;
    t.smoothing_ms = 0.0f;
    TransformState state;
    transform_apply_smoothed(t, 0.0f, 0.01f, state);
    CHECK_NEAR(transform_apply_smoothed(t, 1.0f, 0.01f, state), 1.0, 1e-6);
}
