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
#include <utility>
#include <vector>

struct ImFont;

#include "app/engine.h"
#include "core/library.h"

namespace svj::ui {

// What the front end knows that the engine does not: how long it has been
// running and what the scripted performance calls this moment.
// Controls the hand has claimed from the demo script. The interface is a
// controller here, not a view: the mouse writes into the surface through the
// SAME door MIDI will use, and once a hand owns a control the script never
// writes it again -- exactly what happens when a real fader arrives.
struct HandState {
    std::vector<std::pair<ControlIndex, float>> owned;

    void take(ControlIndex index, float value) {
        for (auto& entry : owned) {
            if (entry.first == index) {
                entry.second = value;
                return;
            }
        }
        owned.emplace_back(index, value);
    }
};

// How the performance screen is arranged. Not decoration: a set has phases,
// and each wants a different thing large. The names are the phases, not the
// widgets -- you pick where you ARE, and the arrangement follows.
enum class Layout : int {
    Booth = 0,   // CABINE: everything reachable, nothing large. The cockpit.
    Stage,       // SCENE: the program dominates; decks reduced to what you glance at.
    Prepare,     // PREPA: no program at all; library wide, filmstrips tall to scrub.
    Sphere,      // 360: the projected view large, with the gaze and a sight frame.
    FullFrame,   // PLEIN CADRE: the program edge to edge, and nothing else.
};

inline constexpr int kLayoutCount = 5;

struct Frame {
    // Which arrangement to draw. Written back when the performer picks another,
    // so the front end owns it across frames and the keyboard can drive it.
    Layout layout = Layout::Booth;

    double elapsed_s = 0.0;
    std::string phase;
    bool follower_mode = true;

    // Where the mixer widgets deposit what the hand did this frame. Null makes
    // the whole surface read-only (a replayed take, for instance).
    HandState* hand = nullptr;

    // The deck a hand is holding THIS frame, written by the filmstrip that owns
    // the drag. The front end releases any deck left in Hand that nobody
    // claimed: a widget can vanish mid-drag (the layout changed under it) and
    // ImGui then never reports the release, which would freeze a deck for the
    // rest of the set. Ownership is asserted every frame rather than assumed to
    // persist, so the stuck state cannot exist.
    const void* scrubbing = nullptr;

    // Live frames for the decks, as textures the render backend understands
    // (SDL_Texture* today). Null draws the honest empty well instead.
    void* tex_a = nullptr;
    void* tex_b = nullptr;

    // The composited program, when the compositor ran this frame: the same
    // pixels the Spout output publishes.
    void* tex_program = nullptr;
    // Deck A's raw equirect, for the 360 layout's source view. Null hides it.
    void* tex_equirect = nullptr;
    unsigned int program_width = 0;
    unsigned int program_height = 0;
    // How many effect passes ran over the program this frame.
    int effect_passes = 0;
    // How many distinct moments of the clip each multi-tap slot read. One means
    // the record is standing still, which is a state worth showing.
    int tap_moments[3] = {};

    // A clip the performer asked to put on a deck this frame. The panel decides
    // WHICH clip and WHICH deck; the front end does the opening, because that
    // is file I/O and resizing GPU targets, neither of which belongs in a view.
    // kNoClip means nothing was asked. A MIDI pad will write the same two
    // fields, which is the reason this is a request rather than a call.
    //
    // The request MUST be served at a frame boundary, not between draw() and
    // ImGui::Render(): loading closes the deck's cache, and the draw list built
    // by draw() still carries those textures as ImTextureIDs. Serving it inline
    // submits destroyed bgfx handles -- which appears to work whenever bgfx
    // hands the same recycled index back, so it fails only sometimes.
    ClipId load_clip = kNoClip;
    DeckTarget load_target = DeckTarget::None;

    // The overlay layer's source. Written back by the panel and read by the
    // front end, which owns the Spout receiver -- opening a receiver is I/O and
    // does not belong in a view, the same rule the clip load follows.
    bool overlay_live = false;
    // Filled by the front end for display: whether a sender is actually there,
    // what it is called, and how much history is held.
    bool live_connected = false;
    std::string live_sender;
    float live_span_s = 0.0f;
    // The receiver landed on this application's OWN Spout output. With no other
    // sender running that is what happens, and the result is a feedback loop --
    // a real technique, but one nobody should discover by accident while
    // wondering why the picture went white.
    bool live_is_self = false;
};

// The three faces the mockup uses. Archivo carries the interface, DM Mono every
// number that has to line up in a column, and the small size does the work
// letter-spaced small caps do on the page -- ImGui has no letter-spacing, so the
// distinction has to come from size and colour instead.
struct Fonts {
    ImFont* sans = nullptr;
    ImFont* mono = nullptr;
    ImFont* small = nullptr;
};

// Set by the front end after the fonts are loaded; null members are tolerated
// everywhere, so a missing font file degrades to the default face rather than
// crashing on a machine where the files were not copied.
extern Fonts g_fonts;

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
void draw(Engine& engine, Frame& frame);

}  // namespace svj::ui
