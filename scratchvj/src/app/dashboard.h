// scratchvj — the terminal dashboard.
//
// A text stand-in for the real interface, and the first thing in this project you
// can actually watch. It shows the same things the ImGui mirror will: the deck
// readouts, the filmstrip with its VRAM window, the surface with ghosted controls
// that have never been touched, and what the mapping engine is producing.
#pragma once

#include <string>
#include <vector>

#include "core/anchor.h"
#include "core/framewindow.h"
#include "core/gestures.h"
#include "core/mapping.h"
#include "core/surface.h"
#include "core/timecode.h"
#include "core/transport.h"
#include "core/videocache.h"

namespace svj {

struct DeckView {
    std::string name;
    const TimecodeState* timecode = nullptr;
    const GestureTracker* gestures = nullptr;
    const Transport* transport = nullptr;
    const FrameWindow* window = nullptr;
    const CacheHeader* clip = nullptr;
};

struct DashboardView {
    double elapsed_s = 0.0;
    std::string phase;
    bool follower_mode = true;
    double bpm = 0.0;
    const Anchor* anchor = nullptr;
    int jump_count = 0;
};

// Renders one frame. `ansi` adds colour and in-place redraw; without it the
// output is plain text, which is what a log or a Windows console without VT
// support wants.
std::string render_dashboard(const DashboardView& view, const DeckView& a, const DeckView& b,
                             const Surface& surface, const MappingEngine& mapping, bool ansi);

}  // namespace svj
