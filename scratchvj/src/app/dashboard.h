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
#include "core/effect.h"
#include "core/framewindow.h"
#include "core/gestures.h"
#include "core/mapping.h"
#include "core/mixer.h"
#include "core/playback.h"
#include "core/modulator.h"
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

    // What the deck's clock resolved: the position actually shown and its signed
    // velocity, after the clip-boundary fold. Read in preference to the
    // transport and the timecode, which are both upstream of that fold and would
    // show a position the clip does not have -- a twelve-second loop reading
    // 00:24.0. Null on a replayed take, which carries no clock.
    const ClockOutput* played = nullptr;
};

struct DashboardView {
    double elapsed_s = 0.0;
    std::string phase;
    bool follower_mode = true;
    double bpm = 0.0;
    const Anchor* anchor = nullptr;
    int jump_count = 0;

    MixWeights weights;

    // The third layer, drawn on its own line rather than folded into the A/B
    // bar: it does not answer to the crossfader, so putting it on that line
    // would suggest it does.
    const Layer* overlay = nullptr;
    const DeckView* overlay_deck = nullptr;
    float overlay_gain = 0.0f;

    const CutDetector* cuts = nullptr;
    const EffectRack* rack = nullptr;
    const ModulatorBank* modulators = nullptr;
};

// Renders one frame. `ansi` adds colour and in-place redraw; without it the
// output is plain text, which is what a log or a Windows console without VT
// support wants.
std::string render_dashboard(const DashboardView& view, const DeckView& a, const DeckView& b,
                             const Surface& surface, const MappingEngine& mapping, bool ansi);

}  // namespace svj
