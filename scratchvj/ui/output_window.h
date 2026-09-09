// scratchvj — the program on a screen of its own.
//
// A VJ set has two screens: the one the performer reads and the one the room
// sees. Until now the second one existed only as Spout, which is right for
// feeding Resolume or Unreal and useless for the ordinary case of a projector
// on the desk's second output.
//
// So this opens a borderless window on a chosen display and draws the very
// texture the interface previews and the Spout sender publishes -- the effect
// rack's output, not the compositor's, so all three show the same picture.
// The draw is a second bgfx swap chain on the same device: no readback, no
// copy through system memory, and therefore no added latency beyond the one
// vsync the second screen costs.
//
// The output geometry -- corner pin, warp mesh, mask -- is applied HERE and
// only here. Spout receives the picture before any of it, so Resolume or
// MadMapper can do their own; the projector receives it after, because for
// the ordinary case there is nothing downstream to do it.
//
// A screen whose shape differs from the program's gets BLACK BARS, never a
// stretch. A projector is a measuring instrument for a VJ: if the software
// silently changed the aspect, every mask and corner pin drawn against it
// would be wrong by that much.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/mesh.h"
#include "core/warp.h"

struct SDL_Window;

namespace svj::ui {

// One screen the machine offers.
struct DisplayInfo {
    std::uint32_t id = 0;  // SDL_DisplayID
    std::string name;
    int width = 0;
    int height = 0;
    float refresh_hz = 0.0f;
    bool primary = false;  // the display the main window is on
};

// Every display, as SDL reports them now. Cheap; call it about once a second
// rather than every frame -- a projector switched on mid-set has to appear.
std::vector<DisplayInfo> displays();

// What shapes the picture on the way to the screen. Null pointers mean
// "none": a null pin is the identity, a null mask covers everything.
struct OutputGeometry {
    const CornerPin* pin = nullptr;
    const WarpMesh* mesh = nullptr;
    bool mesh_enabled = false;
    const Mask* mask = nullptr;

    // True when nothing here changes the picture, so the plain blit serves.
    bool is_identity() const;
};

class OutputWindow {
public:
    ~OutputWindow();

    OutputWindow(const OutputWindow&) = delete;
    OutputWindow& operator=(const OutputWindow&) = delete;
    OutputWindow() = default;

    // Opens on `display_id`, borderless and fullscreen. The two view ids are
    // this window's own: `clear_view` paints the whole screen black and
    // `draw_view` carries the letterboxed picture, so both must come AFTER
    // every view that produces the program.
    bool open(std::uint32_t display_id, std::uint16_t clear_view, std::uint16_t draw_view);
    void close();
    bool ready() const { return window_ != nullptr; }
    std::uint32_t display_id() const { return display_id_; }

    // True when `id` is this window's, so the main loop can tell a close
    // request for the output from one for the instrument.
    bool owns(std::uint32_t window_id) const;

    // Draws `source` (a bgfx texture handle index, 0xFFFF for none) sized
    // `src_w` by `src_h`, letterboxed into the screen, through `geometry`.
    // Call once a frame, before bgfx::frame().
    void present(std::uint16_t source, std::uint32_t src_w, std::uint32_t src_h,
                 const OutputGeometry& geometry = OutputGeometry{});

    int width() const { return width_; }
    int height() const { return height_; }
    const std::string& error() const { return error_; }

    // The grid the warp is tessellated on, per side. 32 keeps a bezier mesh
    // smooth and a homography exact (a homography is exact per triangle only
    // at the vertices, and 32 cells make the error invisible).
    static constexpr int kWarpCells = 32;

private:
    void draw_warped(std::uint16_t source, const OutputGeometry& geometry);

    SDL_Window* window_ = nullptr;
    std::uint32_t display_id_ = 0;
    std::uint16_t clear_view_ = 0;
    std::uint16_t draw_view_ = 0;
    std::uint16_t framebuffer_ = 0xFFFF;
    std::uint16_t program_ = 0xFFFF;
    std::uint16_t warp_program_ = 0xFFFF;
    std::uint16_t sampler_ = 0xFFFF;
    std::uint16_t vertices_ = 0xFFFF;
    int width_ = 0;
    int height_ = 0;
    std::string error_;
};

}  // namespace svj::ui
