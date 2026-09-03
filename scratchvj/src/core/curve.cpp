#include "core/curve.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr float kEpsilon = 1e-9f;

float safe_shape(float shape) { return shape > kEpsilon ? shape : 1.0f; }

}  // namespace

float curve_eval(CurveKind kind, float shape, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float s = safe_shape(shape);

    switch (kind) {
        case CurveKind::Linear:
            return t;
        case CurveKind::Exponential:
            return std::pow(t, s);
        case CurveKind::Logarithmic:
            return std::pow(t, 1.0f / s);
        case CurveKind::SCurve: {
            // t^s / (t^s + (1-t)^s): symmetric, and exactly linear at shape 1.
            const float a = std::pow(t, s);
            const float b = std::pow(1.0f - t, s);
            const float denom = a + b;
            return denom > kEpsilon ? a / denom : t;
        }
    }
    return t;
}

float transform_apply(const Transform& t, float input) {
    const float span = t.in_hi - t.in_lo;
    if (std::fabs(span) < kEpsilon) return t.out_lo;

    float u = (input - t.in_lo) / span;
    u = std::clamp(u, 0.0f, 1.0f);

    // Deadzone is centred, and the surviving travel is rescaled so that the ends
    // remain reachable. Without the rescale a deadzone would shrink the range.
    const float dz = std::clamp(t.deadzone, 0.0f, 0.98f);
    if (dz > kEpsilon) {
        const float half = dz * 0.5f;
        if (u < 0.5f - half) {
            u = u / (0.5f - half) * 0.5f;
        } else if (u > 0.5f + half) {
            u = 0.5f + (u - (0.5f + half)) / (0.5f - half) * 0.5f;
        } else {
            u = 0.5f;
        }
        u = std::clamp(u, 0.0f, 1.0f);
    }

    if (t.invert) u = 1.0f - u;

    const float shaped = curve_eval(t.curve, t.shape, u);
    return t.out_lo + shaped * (t.out_hi - t.out_lo);
}

float transform_apply_smoothed(const Transform& t, float input, float dt_s, TransformState& state) {
    const float target = transform_apply(t, input);

    if (t.smoothing_ms <= kEpsilon || dt_s <= 0.0f) {
        state.value = target;
        state.primed = true;
        return target;
    }

    // Priming with the target avoids an audible or visible sweep from zero the
    // first time a mapping is touched.
    if (!state.primed) {
        state.value = target;
        state.primed = true;
        return target;
    }

    const float tau = t.smoothing_ms * 0.001f;
    const float alpha = 1.0f - std::exp(-dt_s / tau);
    state.value += (target - state.value) * alpha;
    return state.value;
}

}  // namespace svj
