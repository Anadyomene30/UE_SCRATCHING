// scratchvj — the video half of the effect rack, as arithmetic.
//
// core/effect says WHAT each pairing is and how honest it is; this file is what
// the video side actually computes. It exists in core, on the CPU, for the same
// reason core/compose and core/sphere do: the shader that runs it in a set is a
// transcription, and a transcription needs something to be held to. tools/
// fx_check does the holding.
//
// Only the effects that read the CURRENT frame live here. The rack's other half
// -- delay trails, slit scan, datamosh -- reads the clip at SEVERAL positions at
// once, which is a different piece of machinery (multi-tap uploads out of the
// VRAM window) and a different commit. It is worth saying why that split is not
// laziness: a trail done the obvious way is a feedback buffer, and a feedback
// buffer is an INTEGRATOR. Scratch backwards and it keeps accumulating forwards,
// which the first design principle forbids outright. Done properly, trails are a
// tapped delay line on the clip's own timeline -- out = sum of frames at
// position - k*dt -- which is a pure function of position, reverses when the
// hand reverses, and is the same equation as the audio delay it is paired with.
// That is worth building correctly rather than approximating now.
#pragma once

#include <cstdint>
#include <vector>

#include "core/effect.h"

namespace svj {

// What one slot of the rack asks of the picture. The rack's own EffectParams
// carries more than the video side uses; this is the subset, so the shader's
// uniforms and the reference cannot drift apart.
struct VideoFx {
    EffectType type = EffectType::LowPass;
    float mix = 0.0f;     // dry to wet
    float amount = 0.5f;  // the effect's main character
};

// True when this effect only ever reads the frame it is given. The others need
// the clip at several positions and are not implemented here -- asking rather
// than assuming keeps the front end from silently drawing nothing.
bool is_single_frame_effect(EffectType type);

// Applies one effect to a tightly packed RGBA8 image. `out` is resized. A mix of
// zero copies the source through, which is what an idle rack slot must cost.
//
// Sampling is NEAREST and offsets are whole texels, deliberately: it is what
// lets the shader produce the same numbers rather than merely a similar picture,
// and a validated effect is worth more than a fractionally smoother one.
void apply_video_fx(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                    const VideoFx& fx, std::vector<std::uint8_t>& out);

// The blur radius, in whole texels, that `amount` asks for. Exposed because the
// shader needs the identical number and computing it twice from a formula in two
// languages is how the two drift apart.
int blur_step_texels(float amount);

// How many levels posterisation quantises each channel to.
int posterise_levels(float amount);

// How many mirrored wedges the kaleidoscope folds the picture into.
int kaleidoscope_segments(float amount);

}  // namespace svj
