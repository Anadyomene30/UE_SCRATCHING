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

// What the mixer's own switches set. The Elite has a curve control and a
// reverse switch for the crossfader AND another pair for the channel faders,
// which is why there are two of each: a battle mixer's crossfader is sharp
// while its channel faders stay linear, and one setting for both was a lie.
// The Elite's front edge carries three curve/reverse pairs: channel 1, the
// crossfader, channel 2. So each channel fader has its own; `channel` is
// channel 1 (deck A) -- the name predates the second one -- and `channel_b`
// is channel 2. Writing "both" is what the shared mix.fader.* destinations do.
struct MixSettings {
    FaderCurve xfader = FaderCurve::Sharp;
    FaderCurve channel = FaderCurve::Linear;    // channel 1, deck A
    FaderCurve channel_b = FaderCurve::Linear;  // channel 2, deck B
    bool xfader_reverse = false;   // "hamster": A on the right
    bool channel_reverse = false;  // fully open at the bottom
    bool channel_b_reverse = false;
};

// A curve from a 0..1 control: four positions over the travel, so a knob or a
// four-way button both land on one of the four curves.
FaderCurve curve_from_unit(float value01);
const char* curve_name(FaderCurve curve);

// Gains for the two decks at a crossfader position, 0 fully on A, 1 fully on B.
MixWeights crossfader_weights(float position, FaderCurve curve);

// A channel fader through its own curve. Linear is the fader itself; the
// other curves shape its travel the way the hardware's switch does.
float channel_gain(float fader, FaderCurve curve);

// The crossfader combined with the two channel faders, which is what the video
// mixer and the audio mixer both actually use.
MixWeights mix_weights(float crossfader, float fader_a, float fader_b, FaderCurve curve);
MixWeights mix_weights(float crossfader, float fader_a, float fader_b,
                       const MixSettings& settings);

struct Layer {
    bool enabled = false;
    float opacity = 1.0f;
    BlendMode blend = BlendMode::Normal;
};

// The whole stack: the two decks, and one layer over them.
//
// One, not N. A logo, a text card, a mask or a running texture is what a set
// needs on top of two scratched decks; a clip matrix and layer groups would make
// this a VJ compositor instead of an instrument, and the answer to those cases is
// to send the mix out over Spout or NDI and let Resolume do them.
struct StackWeights {
    float a = 0.0f;
    float b = 0.0f;
    float overlay = 0.0f;
};

// The crossfader deliberately does NOT reach the overlay.
//
// A logo that vanished every time the fader crossed would be worse than no logo,
// and a mask that opened mid-transform would show the thing it exists to hide.
// The third layer sits ABOVE the crossfader: it is the part of the picture that
// survives the transition happening underneath it, and its only gain is its own.
StackWeights stack_weights(float crossfader, float fader_a, float fader_b,
                           FaderCurve curve, const Layer& overlay);
StackWeights stack_weights(float crossfader, float fader_a, float fader_b,
                           const MixSettings& settings, const Layer& overlay);

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
