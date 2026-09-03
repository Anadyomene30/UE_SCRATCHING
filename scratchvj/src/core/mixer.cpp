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

MixWeights mix_weights(float crossfader, float fader_a, float fader_b, FaderCurve curve) {
    MixWeights w = crossfader_weights(crossfader, curve);
    w.a *= std::clamp(fader_a, 0.0f, 1.0f);
    w.b *= std::clamp(fader_b, 0.0f, 1.0f);
    return w;
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
