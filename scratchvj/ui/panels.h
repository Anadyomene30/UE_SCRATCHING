// scratchvj — the interface, drawn from the engine.
//
// A view and nothing else. It reads `Engine` and paints it; it holds no state
// of its own beyond what a widget needs to be drawn, and it decides nothing. If
// a behaviour ever seems to want to live here, it belongs in `core/` with a test
// instead -- that boundary is what has kept the whole instrument testable
// without a window, and adding a window is no reason to give it up.
//
// The layout follows the mockup in `design/`, and so do the four decisions that
// mockup exists to fix: ghost controls, the Phase link indicator, the VRAM
// window drawn on the filmstrip, and the unlinked-effect marker.
#pragma once

#include <string>

#include "app/engine.h"

namespace svj::ui {

// What the front end knows that the engine does not: how long it has been
// running and what the scripted performance calls this moment.
struct Frame {
    double elapsed_s = 0.0;
    std::string phase;
    bool follower_mode = true;
};

// Applies the palette and metrics the mockup fixes. Call once, after the ImGui
// context exists.
void apply_style();

// Draws one frame of the whole interface into the current ImGui context.
//
// Takes the engine by non-const reference for one reason: the overlay layer is
// the only thing on screen a performer owns outright rather than receiving from
// the platters, so it is the only thing this view is allowed to write. Every
// other panel reads. When there is a MIDI surface, that same rule holds -- the
// controls move the engine, the interface only ever shows what moved.
void draw(Engine& engine, const Frame& frame);

}  // namespace svj::ui
