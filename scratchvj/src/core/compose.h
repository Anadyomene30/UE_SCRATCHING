// scratchvj — compositing the program output on the CPU.
//
// The program did not exist until now: the mixer computed WEIGHTS and the
// interface drew each deck separately, but nothing ever produced the one image
// that goes to the projector, to Spout, to NDI. This is that image.
//
// It is a CPU compositor, and that is a deliberate stopgap with the same seam as
// the CPU BC1 decode: bgfx does this exact arithmetic in a fragment shader on
// the same inputs. Doing it here first means the blend math lives in core, has
// tests, and the shader can be checked against a reference instead of against an
// impression (tools/gpu_check).
//
// Layers are accumulated bottom-up into an opaque RGBA8 frame. Sources may be
// any size; they are sampled nearest-neighbour at the output's resolution --
// scaling quality is the GPU's job, and pretending otherwise here would just
// be a slow blur.
#pragma once

#include <cstdint>
#include <vector>

#include "core/mixer.h"

namespace svj {

// One source plugged into the compositor.
struct ComposeLayer {
    const std::uint8_t* rgba = nullptr;  // tightly packed RGBA8
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    float gain = 1.0f;  // 0 leaves the canvas untouched, 1 applies the mode fully
    BlendMode blend = BlendMode::Normal;
};

// How deck B arrives over deck A along the crossfader. `gain_a` and `gain_b`
// are the mixer's weights (curves and channel faders already applied);
// `position` is the crossfader itself, 0 on A and 1 on B, after reverse.
//
// Fade and Additive are the two that read the weights: B is eased in by its
// weight, over A (Fade) or added to A (Additive -- the constant-power one a
// scratch mixer's sharp curve wants). The others are transitions in the
// video sense and read the POSITION, so a sharp curve does not collapse a
// wipe into a cut: the curve is the law of the faders, the transition is how
// one picture becomes the other.
struct Crossfade {
    Transition transition = Transition::Additive;
    float gain_a = 1.0f;
    float gain_b = 0.0f;
    float position = 0.0f;
};

// Fills the canvas with opaque black. The program starts dark, not undefined.
void clear_program(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                   std::uint32_t height);

// Blends one layer over what the canvas already holds. A null source or a zero
// gain is a no-op rather than an error: a deck with nothing loaded is silent,
// not broken.
void accumulate_layer(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                      std::uint32_t height, const ComposeLayer& layer);

// The two decks through the crossfader, over a black canvas. Every transition
// shows A alone at position 0 with gain_a 1, and B alone at position 1 with
// gain_b 1: a transition that did not is a broken crossfader. A null deck is
// black. The layers' own `gain` and `blend` are ignored; the Crossfade says.
void compose_decks(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                   std::uint32_t height, const ComposeLayer& deck_a,
                   const ComposeLayer& deck_b, const Crossfade& crossfade);

// The transition a 0..1 control lands on: nine steps over the travel.
Transition transition_from_unit(float value01);
const char* transition_name(Transition transition);

}  // namespace svj
