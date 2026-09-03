// scratchvj — the transformation stage of the mapping engine.
//
// Sits between a source (a knob, or a value derived from the platter) and a
// destination (an effect parameter, a 360 axis, an OSC address). Every mapping
// owns one of these, which is what lets the same physical control drive different
// destinations with different responses.
#pragma once

namespace svj {

enum class CurveKind {
    Linear,
    Exponential,   // slow at first, then steep — good for effect intensity
    Logarithmic,   // steep at first, then slow — good for filter frequency
    SCurve,        // eased at both ends — good for crossfades and camera moves
};

struct Transform {
    CurveKind curve = CurveKind::Linear;
    float shape = 2.0f;  // steepness for the non-linear curves; 1.0 is linear

    float in_lo = 0.0f;
    float in_hi = 1.0f;
    float out_lo = 0.0f;
    float out_hi = 1.0f;

    bool invert = false;

    // Fraction of the input span, centred on its midpoint, that snaps to the
    // centre. Lets a detented filter knob sit reliably at neutral. The remaining
    // travel is rescaled so the output still reaches both ends.
    float deadzone = 0.0f;

    // One-pole smoothing time constant. Zero passes values through untouched.
    float smoothing_ms = 0.0f;
};

// Carries the smoothing filter's memory. One per active mapping.
struct TransformState {
    float value = 0.0f;
    bool primed = false;

    void reset() {
        value = 0.0f;
        primed = false;
    }
};

// Applies range, deadzone, curve and inversion. Pure — no smoothing, no state.
float transform_apply(const Transform& t, float input);

// As above, then one-pole smoothing over `dt_s` seconds. The first call primes
// the filter with the target value so a mapping never sweeps up from zero.
float transform_apply_smoothed(const Transform& t, float input, float dt_s, TransformState& state);

// Shapes a 0..1 value by a curve. Exposed for reuse and for testing.
float curve_eval(CurveKind kind, float shape, float t);

}  // namespace svj
