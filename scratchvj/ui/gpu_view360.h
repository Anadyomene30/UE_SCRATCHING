// scratchvj — the 360 view pass.
//
// Renders one deck's equirectangular texture into a flat view -- perspective,
// little planet or fisheye -- with fs_view360.sc, the GPU transcription of
// core/sphere that tools/sphere_check holds to its reference. The output is an
// ordinary RGBA target the compositor and the interface consume like any other
// deck picture: downstream, a 360 deck IS a flat deck, which is what lets the
// program stay one pipeline.
//
// This same pass is what a headset will run twice per frame -- one eye each,
// gaze taken from the head pose instead of the knobs. Nothing here will change
// for that beyond who supplies the SphereView.
#pragma once

#include <cstdint>

#include "core/sphere.h"

namespace svj::ui {

class View360Gpu {
public:
    // `view_id` must order BEFORE the compositor's view.
    bool init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id);
    void destroy();
    bool ready() const { return ready_; }

    // Reprojects `equirect` (a bgfx texture handle index) through `view`.
    void render(std::uint16_t equirect, const SphereView& view);

    // The flat result: a bgfx handle index for the compositor, and an
    // ImTextureID-compatible pointer for the interface.
    std::uint16_t texture_index() const { return target_; }
    void* imgui_texture() const;

private:
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint16_t view_id_ = 0;
    bool ready_ = false;

    std::uint16_t target_ = 0xFFFF;
    std::uint16_t framebuffer_ = 0xFFFF;
    std::uint16_t program_ = 0xFFFF;
    std::uint16_t gaze_uniform_ = 0xFFFF;
    std::uint16_t frame_uniform_ = 0xFFFF;
    std::uint16_t sampler_ = 0xFFFF;
    std::uint16_t vertices_ = 0xFFFF;
};

}  // namespace svj::ui
