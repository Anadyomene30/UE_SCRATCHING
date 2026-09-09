#include "core/mixer.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kHalfPi = 1.57079632679489661923;

// Travel over which a channel goes from silent to full. Small enough that a flick
// of the fader is a cut rather than a fade -- which is the whole point of the
// sharp curve, and the reason a transform is playable.
constexpr float kSharpKnee = 0.02f;

float ramp(float distance, float knee) {
    if (knee <= 0.0f) return distance > 0.0f ? 1.0f : 0.0f;
    return std::clamp(distance / knee, 0.0f, 1.0f);
}

}  // namespace

MixWeights crossfader_weights(float position, FaderCurve curve) {
    const float x = std::clamp(position, 0.0f, 1.0f);
    MixWeights w;

    switch (curve) {
        case FaderCurve::Smooth:
            // Constant power: the sum of squares stays one, so a blend does not
            // dip in the middle.
            w.a = static_cast<float>(std::cos(x * kHalfPi));
            w.b = static_cast<float>(std::sin(x * kHalfPi));
            break;

        case FaderCurve::Linear:
            w.a = 1.0f - x;
            w.b = x;
            break;

        case FaderCurve::Sharp:
            // Both channels are open across the middle; only the last sliver at
            // each end closes one. That wide open zone is what a crab needs.
            w.a = ramp(1.0f - x, kSharpKnee);
            w.b = ramp(x, kSharpKnee);
            break;

        case FaderCurve::Cut:
            w.a = x < 0.5f ? 1.0f : 0.0f;
            w.b = x < 0.5f ? 0.0f : 1.0f;
            break;
    }
    return w;
}

FaderCurve curve_from_unit(float value01) {
    const float x = std::clamp(value01, 0.0f, 1.0f);
    if (x < 0.25f) return FaderCurve::Smooth;
    if (x < 0.5f) return FaderCurve::Linear;
    if (x < 0.75f) return FaderCurve::Sharp;
    return FaderCurve::Cut;
}

const char* curve_name(FaderCurve curve) {
    switch (curve) {
        case FaderCurve::Smooth: return "douce";
        case FaderCurve::Linear: return "lin\xC3\xA9" "aire";
        case FaderCurve::Sharp: return "sharp";
        case FaderCurve::Cut: return "cut";
    }
    return "?";
}

float channel_gain(float fader, FaderCurve curve) {
    const float x = std::clamp(fader, 0.0f, 1.0f);
    switch (curve) {
        case FaderCurve::Smooth:
            // A gentle law: most of the travel does something, the top eases.
            return static_cast<float>(std::sin(x * kHalfPi));
        case FaderCurve::Sharp: return ramp(x, kSharpKnee);
        case FaderCurve::Cut: return x < 0.5f ? 0.0f : 1.0f;
        case FaderCurve::Linear:
        default: return x;
    }
}

MixWeights mix_weights(float crossfader, float fader_a, float fader_b, FaderCurve curve) {
    MixWeights w = crossfader_weights(crossfader, curve);
    w.a *= std::clamp(fader_a, 0.0f, 1.0f);
    w.b *= std::clamp(fader_b, 0.0f, 1.0f);
    return w;
}

MixWeights mix_weights(float crossfader, float fader_a, float fader_b,
                       const MixSettings& settings) {
    const float x = settings.xfader_reverse ? 1.0f - std::clamp(crossfader, 0.0f, 1.0f)
                                            : crossfader;
    const float fa = settings.channel_reverse ? 1.0f - std::clamp(fader_a, 0.0f, 1.0f) : fader_a;
    const float fb = settings.channel_b_reverse ? 1.0f - std::clamp(fader_b, 0.0f, 1.0f) : fader_b;
    MixWeights w = crossfader_weights(x, settings.xfader);
    w.a *= channel_gain(fa, settings.channel);
    w.b *= channel_gain(fb, settings.channel_b);
    return w;
}

StackWeights stack_weights(float crossfader, float fader_a, float fader_b,
                           const MixSettings& settings, const Layer& overlay) {
    const MixWeights below = mix_weights(crossfader, fader_a, fader_b, settings);
    StackWeights stack;
    stack.a = below.a;
    stack.b = below.b;
    // The crossfader is absent from this line on purpose; see the header.
    stack.overlay = overlay.enabled ? std::clamp(overlay.opacity, 0.0f, 1.0f) : 0.0f;
    return stack;
}

StackWeights stack_weights(float crossfader, float fader_a, float fader_b,
                           FaderCurve curve, const Layer& overlay) {
    const MixWeights below = mix_weights(crossfader, fader_a, fader_b, curve);

    StackWeights stack;
    stack.a = below.a;
    stack.b = below.b;
    // The crossfader is absent from this line on purpose; see the header.
    stack.overlay = overlay.enabled ? std::clamp(overlay.opacity, 0.0f, 1.0f) : 0.0f;
    return stack;
}

void CutDetector::reset() {
    crossings_.clear();
    have_previous_ = false;
    rate_ = 0.0f;
}

void CutDetector::update(double time_s, float crossfader) {
    // "Open" means past the midpoint, with hysteresis so a fader resting near the
    // middle does not chatter a stream of imaginary cuts.
    const bool open = have_previous_
                          ? (open_ ? crossfader > 0.5f - threshold_ * 0.5f
                                   : crossfader > 0.5f + threshold_ * 0.5f)
                          : crossfader > 0.5f;

    if (have_previous_ && open != open_) crossings_.push_back(time_s);
    open_ = open;
    have_previous_ = true;

    const double cutoff = time_s - window_s_;
    while (!crossings_.empty() && crossings_.front() < cutoff) {
        crossings_.erase(crossings_.begin());
    }
    rate_ = static_cast<float>(crossings_.size()) / window_s_;
}

}  // namespace svj
