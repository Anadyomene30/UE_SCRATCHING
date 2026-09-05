// scratchvj — compositing the program output on the CPU.
//
// The program did not exist until now: the mixer computed WEIGHTS and the
// interface drew each deck separately, but nothing ever produced the one image
// that goes to the projector, to Spout, to NDI. This is that image.
//
// It is a CPU compositor, and that is a deliberate stopgap with the same seam as
// the CPU BC1 decode: bgfx will do this exact arithmetic in a fragment shader on
// the same inputs. Doing it here first means the blend math lives in core, has
// tests, and the shader can be checked against a reference instead of against an
// impression. At program resolutions (the analysis pass caps width at 1024) the
// cost is a few milliseconds, which the stopgap can afford.
//
// Layers are accumulated bottom-up into an opaque RGBA8 frame. Sources may be
// any size; they are sampled nearest-neighbour at the output's resolution --
// scaling quality is the GPU's job later, and pretending otherwise here would
// just be a slow blur.
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

// Fills the canvas with opaque black. The program starts dark, not undefined.
void clear_program(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                   std::uint32_t height);

// Blends one layer over what the canvas already holds. A null source or a zero
// gain is a no-op rather than an error: a deck with nothing loaded is silent,
// not broken.
void accumulate_layer(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                      std::uint32_t height, const ComposeLayer& layer);

}  // namespace svj
