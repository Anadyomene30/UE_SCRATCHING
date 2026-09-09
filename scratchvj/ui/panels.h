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
#include "config/settings_io.h"
#include "core/library.h"
#include "core/profile.h"
#include "core/scope.h"

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

// The screens, in tab order. One axis of navigation: a set lives in JOUER,
// the others are preparation and configuration. The five "layouts" this
// replaces (cabine, scène, prépa, 360, plein cadre) were a second axis on top
// of the tabs, and nobody could say where they were.
enum class Screen : int { Play = 0, Library, Effects, Table, Output, Settings };

struct Frame {
    // Written by the panel when a click asks for another screen (the rack
    // summary on JOUER opens EFFETS), consumed by draw() on the next pass.
    Screen screen_request = Screen::Play;
    bool screen_requested = false;

    // JOUER's two switches, which the keyboard also drives (F and B). Owned
    // here so they survive the frame.
    bool full_frame = false;   // the program edge to edge, nothing else
    bool rail_open = true;     // the library rail beside the decks

    double elapsed_s = 0.0;
    std::string phase;

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
    // With `keep_position`, the deck holds its played position across the
    // load: the deck header's 2D | 360 selector reloads the SAME clip with a
    // different projection, and the picture must not jump to the head.
    bool load_keep_position = false;

    // Which library clip each layer is showing, so the deck header can write a
    // projection back to the library entry. kNoClip on a deck that holds no
    // analysed clip (the demo's fabricated ones included).
    ClipId clip_on_a = kNoClip;
    ClipId clip_on_b = kNoClip;
    ClipId clip_on_overlay = kNoClip;

    // --- the library: imports, analyses, the queue -------------------------------
    // All requests, served at the frame boundary like the clip load: a file
    // dialog, a folder walk and an analysis are I/O, and none of it belongs in
    // a view. The panel says what was asked; the front end does it.
    bool import_files_request = false;   // open the file picker
    bool import_folder_request = false;  // open the folder picker, import what it holds
    bool add_folder_request = false;     // settings: pick a folder to keep watching
    bool rescan_request = false;         // walk the library folders again
    ClipId analyse_clip = kNoClip;       // analyse this one now
    bool analyse_all_request = false;    // analyse everything not yet analysed
    bool load_next_request = false;      // take the head of the queue
    // The panel changed something library.json remembers (a projection, a
    // crate). The front end writes the file.
    bool library_dirty = false;
    char library_search[64] = {};
    int library_crate = -1;              // -1 shows every clip
    int library_filter = 0;              // 0 all, 1 flat, 2 spheres, 3 alpha
    ClipId library_selected = kNoClip;   // the inspector's clip
    // One ImTextureID per ClipId (null: no picture yet), filled by the front
    // end from the caches' thumbnails.
    std::vector<void*> thumbnails;

    // --- a take: the control stream on disk --------------------------------------
    // REC on the status bar. The front end owns the file; the panel asks.
    bool take_recording = false;
    bool take_toggle_request = false;
    std::string take_name;            // the file being written, for display
    std::uint32_t take_records = 0;

    // Replaying a take: the front end lists takes/ on request, opens the one
    // asked for, drives the surface and the decks from it until it ends.
    bool take_list_request = false;
    std::vector<std::string> takes;
    std::string take_replay_request;   // a path to start replaying
    bool take_replay_stop = false;
    bool take_replaying = false;
    std::string take_replay_name;
    float take_replay_progress = 0.0f;

    // Deck A's anchor against Serato: "here, now". Served by the front end.
    bool anchor_request = false;

    // The mapping list changed (a row added, edited, dropped): the front end
    // rebinds the engine and writes mapping.json.
    bool mappings_dirty = false;
    int mapping_selected = -1;
    // "Bouger un contrôle": the next control that moves names the selected
    // mapping's source. Filled by the front end from the surface.
    bool mapping_listen = false;

    // Which tool the SORTIE screen is on: 0 pin, 1 mesh, 2 mask.
    int output_tool = 0;
    int mask_selected = -1;

    // --- the pads ---------------------------------------------------------------
    // What a pad does on each deck: 0 cues, 1 clips (the matrix), 2 loops.
    int pad_mode_a = 0;
    int pad_mode_b = 0;
    // What the pads and the loop row asked this frame. Merged by the front
    // end into the next engine step, through the same DeckCommands a mapped
    // pad writes, so a click and a pad cannot disagree.
    DeckCommands commands_a;
    DeckCommands commands_b;
    // Filled by the front end for display.
    std::size_t analysis_pending = 0;
    bool analysis_busy = false;
    bool share_open = false;             // the Spout sender is publishing
    // The last thing that went wrong, in one line, so a failed analysis or an
    // unreadable file is said on screen rather than lost in a stderr nobody
    // can read from a WIN32 application.
    std::string last_error;

    // The desk's settings, edited on the RÉGLAGES screen and written by the
    // front end when `settings_dirty` is set.
    DeskSettings* settings = nullptr;
    bool settings_dirty = false;

    // --- the output screen ---------------------------------------------------
    // Every display the machine offers, refilled about once a second so a
    // projector switched on mid-set appears without a restart. Filled by the
    // front end; the panel picks one and asks.
    struct DisplayView {
        std::uint32_t id = 0;
        std::string name;
        int width = 0;
        int height = 0;
        float refresh_hz = 0.0f;
        bool primary = false;
        bool is_output = false;  // the program is on this one right now
    };
    std::vector<DisplayView> displays;
    bool output_open = false;
    std::string output_error;
    // Requests, served at the frame boundary: opening a window and a swap
    // chain is I/O and does not belong in a view.
    std::uint32_t open_output_display = 0;  // non-zero opens the program there
    bool close_output_request = false;

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

    // Deck A's platter source: the demo script, or the real carrier read off
    // an audio input through core/quadrature. Written by the panel, served by
    // the front end at the frame boundary, like the other two requests above --
    // opening an audio device is I/O and does not belong in a view.
    bool deck_a_live = false;
    // Filled by the front end for display.
    bool platter_connected = false;
    std::string platter_endpoint;
    float platter_level = 0.0f;
    bool platter_locked = false;
    std::uint32_t platter_slews = 0;
    // The carrier as a shape, for the calibration scope in the drawer: the
    // measurement, and the points to draw it from, interleaved x,y. Copied
    // rather than pointed at -- a view owns nothing that can outlive a frame.
    ScopeReading platter_figure;
    std::vector<float> platter_trace;

    // --- the mixer, over MIDI -------------------------------------------------
    // Filled by the front end, which owns the port.
    bool midi_connected = false;
    std::string midi_port;
    std::uint64_t midi_messages = 0;
    std::size_t midi_bound = 0;   // controls with a binding
    std::size_t midi_total = 0;   // controls declared

    // The rig, for the surface panel to draw and the settings to edit: one
    // entry per configured device, in device-index order.
    struct RigDeviceView {
        const DeviceProfile* profile = nullptr;  // null when the name is unknown
        std::string profile_name;
        std::string port;                        // the fragment, or the open port's name
        bool connected = false;
        char deck = 'a';
    };
    std::vector<RigDeviceView> rig;
    std::vector<std::string> midi_ports;      // what the machine lists, for the settings
    std::vector<std::string> profile_names;   // built-in and loaded, for the settings
    bool rig_reconfigure_request = false;     // the devices changed in the settings

    // MIDI learn, driven from the panel and served by the front end. `learning`
    // reflects whether a run is in progress; the two requests below are edges
    // the front end consumes and clears, the same shape as the clip load.
    // A run is per DEVICE: sweeping the mixer must not bind a turntable's pad
    // that happened to be pressed.
    bool learning = false;
    int learn_device = -1;        // which device the run is (or is asked) for
    std::string learn_prompt;     // what to sweep now
    std::size_t learn_remaining = 0;
    bool learn_start = false;     // begin a run over learn_device's profile
    bool learn_skip = false;      // leave the current control unbound
    bool learn_cancel = false;    // stop, keeping what was learned so far

    // The row a mapped "library next/prev" walks, and a pad loads from.
    int library_cursor = -1;

    // Whether the scripted performance is running. It animates whatever nobody
    // has taken over, which is what lets the instrument be plugged in mid-set
    // without the screen going dark -- but while testing by hand it hides your
    // own changes under its own. Off freezes the decks where they are and
    // leaves every control alone.
    bool script_running = false;
    // Whether this session has the demo's fabricated clips. Without them the
    // DÉMO button has nothing to animate, so it is not offered.
    bool demo_content = false;

    // What the front end just did by itself, said on screen for a few seconds:
    // a drop that analysed and landed on a deck must not look like nothing
    // happened. Cleared by the front end.
    std::string notice;
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
