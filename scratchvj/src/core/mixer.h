// scratchvj — the crossfader, the transitions and the layer stack.
//
// The crossfader curve is not a cosmetic setting on a battle mixer, it is what
// makes a transform or a crab possible at all. On a SHARP curve a channel reaches
// full within a couple of percent of travel, so a flick of the fader is a clean
// cut; on a smooth curve the same flick is a fade and the gesture does not exist.
// That is why the curve lives here with a test on the knee rather than being left
// to whatever the graphics happen to do.
#pragma once

#include <cstdint>
#include <vector>

namespace svj {

enum class FaderCurve : std::uint8_t {
    Smooth,   // constant power, for blends
    Linear,
    Sharp,    // battle curve: full within a few percent
    Cut,      // hard switch at the midpoint
};

// How a deck is composited over what is beneath it.
enum class BlendMode : std::uint8_t {
    Normal,
    Add,
    Multiply,
    Screen,
    Alpha,
};

// What the crossfader does between the two decks.
enum class Transition : std::uint8_t {
    Cut,
    Fade,
    Additive,
    Multiply,
    Screen,
    LumaWipe,
    GeoWipe,
    RgbSplit,
    ZoomBlur,
};

struct MixWeights {
    float a = 0.0f;
    float b = 0.0f;
};

// Gains for the two decks at a crossfader position, 0 fully on A, 1 fully on B.
MixWeights crossfader_weights(float position, FaderCurve curve);

// The crossfader combined with the two channel faders, which is what the video
// mixer and the audio mixer both actually use.
MixWeights mix_weights(float crossfader, float fader_a, float fader_b, FaderCurve curve);

struct Layer {
    bool enabled = false;
    float opacity = 1.0f;
    BlendMode blend = BlendMode::Normal;
};

// Detects a transform or crab: repeated fast cuts of the crossfader. Reported as
// a rate rather than a flag so it can drive an effect continuously.
class CutDetector {
public:
    explicit CutDetector(float window_s = 1.0f, float threshold = 0.25f)
        : window_s_(window_s), threshold_(threshold) {}

    void update(double time_s, float crossfader);
    void reset();

    // Crossings of the middle per second over the window.
    float cuts_per_second() const { return rate_; }

    // True while the fader is being worked fast enough to be a transform.
    bool transforming() const { return rate_ >= 4.0f; }

private:
    float window_s_;
    float threshold_;
    std::vector<double> crossings_;
    bool open_ = false;
    bool have_previous_ = false;
    float rate_ = 0.0f;
};

}  // namespace svj
