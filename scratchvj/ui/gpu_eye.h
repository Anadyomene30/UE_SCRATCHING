// scratchvj — one eye of the sphere, rendered for a headset.
//
// The headset counterpart of View360Gpu: same equirect input, same output
// format, but the view comes from a head pose and a four-tangent field of view
// instead of knobs and one half-angle. fs_view360_eye.sc is the transcription of
// core/headset, and tools/eye_check holds it there.
//
// Deliberately NOT folded into View360Gpu. The two differ in what they are for:
// the flat pass renders one view whose size follows the window and whose
// projection the performer picks, while this one renders a fixed-size view per
// eye, twice a frame, into whatever the runtime handed back. Sharing a class
// would mean each carrying the other's parameters.
//
// It renders into its OWN target rather than straight into an OpenXR swapchain
// image, so the pass is testable with no headset present -- which is how it was
// validated. Handing the result to a swapchain is the session's job.
#pragma once

#include <cstdint>

#include "core/headset.h"
#include "core/sphere.h"

namespace svj::ui {

class EyeGpu {
public:
    bool init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id);
    void destroy();
    bool ready() const { return ready_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

    // Reprojects `equirect` for one eye. `world` supplies the performer's
    // rotation of the sphere; only its yaw, pitch and roll are read, because a
    // headset dictates its own projection and field of view.
    void render(std::uint16_t equirect, const SphereView& world, const EyeView& eye);

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
    std::uint16_t fov_uniform_ = 0xFFFF;
    std::uint16_t rot_uniform_ = 0xFFFF;
    std::uint16_t gaze_uniform_ = 0xFFFF;
    std::uint16_t sampler_ = 0xFFFF;
    std::uint16_t vertices_ = 0xFFFF;
};

}  // namespace svj::ui
