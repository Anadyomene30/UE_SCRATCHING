// scratchvj — the program, composited on the GPU.
//
// Renders A + B + overlay into an offscreen target with the fs_program shader
// (a transcription of core/compose, held to it by tools/gpu_check), shows that
// target in the interface, and reads it back for the outputs that need CPU
// bytes -- Spout today, NDI tomorrow. The readback lands two frames late by
// GPU nature; for a video feed that is invisible, and the preview shown in the
// interface is the render target itself, not the readback, so what the
// performer sees has no added latency at all.
//
// This retires the CPU compositor from the hot path. core/compose stays, as
// the tested reference the shader answers to.
#pragma once

#include <cstdint>
#include <vector>

namespace svj::ui {

class ProgramGpu {
public:
    // View ids this pass owns: `view_id` draws into the offscreen target and
    // must come BEFORE the interface's view; `blit_view_id` carries the
    // readback copy and must come AFTER it.
    bool init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id,
              std::uint16_t blit_view_id);
    void destroy();
    bool ready() const { return ready_; }

    // Composites one frame. Texture arguments are bgfx handle indices
    // (0xFFFF = absent, sampled as black), gains 0..1, `overlay_mode` a
    // core/mixer BlendMode value.
    void render(std::uint16_t deck_a, std::uint16_t deck_b, std::uint16_t overlay,
                float gain_a, float gain_b, float gain_overlay, int overlay_mode);

    // Queues a copy of `source` for readback. Separate from render() because
    // the picture that leaves the machine is the one the EFFECT RACK produced,
    // not the one the compositor did -- reading the compositor's own target
    // would quietly send Spout a different image from the one on screen.
    void queue_readback(std::uint16_t source);

    // The render target, for the interface's preview. ImTextureID-compatible.
    void* imgui_texture() const;
    // The same target as a bgfx handle index, for the effect rack to read.
    std::uint16_t texture_index() const { return target_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

    // Hands back the newest completed readback, or null when none has landed
    // yet. `current_frame` is bgfx::frame()'s return value this iteration.
    const std::uint8_t* completed_frame(std::uint32_t current_frame);

private:
    struct Slot {
        std::vector<std::uint8_t> pixels;
        std::uint32_t ready_frame = 0;
        bool pending = false;
    };

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint16_t view_id_ = 0;
    std::uint16_t blit_view_id_ = 0;
    bool ready_ = false;

    // Handle indices rather than bgfx types, so this header pulls no bgfx in.
    std::uint16_t target_ = 0xFFFF;
    std::uint16_t framebuffer_ = 0xFFFF;
    std::uint16_t readback_ = 0xFFFF;
    std::uint16_t program_ = 0xFFFF;
    std::uint16_t uniform_ = 0xFFFF;
    std::uint16_t samplers_[3] = {0xFFFF, 0xFFFF, 0xFFFF};
    std::uint16_t vertices_ = 0xFFFF;
    std::uint16_t black_ = 0xFFFF;  // stands in for an absent layer

    Slot slots_[2];
    int next_slot_ = 0;
};

}  // namespace svj::ui
