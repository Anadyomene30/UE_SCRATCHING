// scratchvj — the multi-tap pass: trails and slit scan, per deck.
//
// These effects sit on a DECK rather than on the program, and that is forced
// rather than chosen. They need the clip at several moments; the program at
// several moments would be a history buffer, which is the integrator the first
// design principle forbids. Only a deck can hand back its own clip at an
// arbitrary position, so only a deck can carry them.
//
// The pass therefore runs BEFORE the 360 reprojection and before compositing:
// deck frames -> taps -> 360 -> compose -> single-frame effects -> screen.
// Trailing in equirect space rather than in view space is a side effect of that
// order and the right one anyway -- the trail lives in the sphere, so turning
// the gaze reveals it instead of dragging it along.
#pragma once

#include <cstdint>

#include "core/videotaps.h"

namespace svj::ui {

class TapsGpu {
public:
    bool init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id);
    void destroy();
    bool ready() const { return ready_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

    // Renders `taps` (a bgfx texture ARRAY handle index) through `plan` for
    // `unit`, and returns the handle index holding the result. Returns
    // `fallback` untouched when there is nothing to do -- an inactive unit, a
    // collapsed plan (a stopped record has no trail), or an effect this pass
    // does not serve.
    std::uint16_t render(std::uint16_t taps, std::uint16_t fallback,
                         const EffectUnit& unit, const TapPlan& plan);

private:
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint16_t view_id_ = 0;
    bool ready_ = false;

    std::uint16_t target_ = 0xFFFF;
    std::uint16_t framebuffer_ = 0xFFFF;
    std::uint16_t program_ = 0xFFFF;
    std::uint16_t sampler_ = 0xFFFF;
    std::uint16_t taps_uniform_ = 0xFFFF;
    std::uint16_t weights_uniform_ = 0xFFFF;
    std::uint16_t vertices_ = 0xFFFF;
};

}  // namespace svj::ui
