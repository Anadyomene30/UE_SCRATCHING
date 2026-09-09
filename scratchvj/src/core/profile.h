// scratchvj — a controller, described as data.
//
// A profile says what a device HAS (its controls, with ids and kinds), how its
// endless encoders speak, and how the interface should DRAW it (groups of
// controls with a title and an accent). It says nothing about which CC or note
// drives what: that is a binding, learned on the desk or shipped alongside the
// profile as a starting point, and it stays separate so the same profile
// serves a mixer whose map was learned and one whose map was copied from a
// published protocol.
//
// The Elite and the RP-8000 are tables in profiles_builtin.cpp; other devices
// (APC40, Push) are JSON files in profiles/, read by config/profile_io. Both
// arrive here as the same struct, so nothing downstream knows the difference.
//
// Ids are hierarchical ("ch1.eq.hi", "pad.elite.a.3") so the wire schema, the
// mapping file and the drawing agree without a second table of what belongs
// where. A profile is a CHECKLIST as much as a description: every control it
// names is what --midi-learn asks to be swept, and what the state packet
// carries, in this order.
#pragma once

#include <string>
#include <vector>

#include "core/learn.h"
#include "core/surface.h"

namespace svj {

struct ProfileControl {
    std::string id;
    ControlKind kind = ControlKind::Knob;
    EncoderMode encoder = EncoderMode::Absolute;
    // A control the hardware may not report over MIDI (the Elite's fader curve
    // buttons, whose MIDI is unmeasured). Learn passes over it without leaving
    // the session unfinished, and the drawing marks it.
    bool optional = false;
};

// How one cluster of controls is drawn: a title, an accent, the ids in
// drawing order, and WHERE it sits on the panel. `columns` lays pads and
// buttons in a grid; zero means a row.
//
// The position is what turns a list of knobs into a picture of the mixer.
// Drawn as one long scrolling row, a control surface is unreadable: the hand
// knows where the filter knob is by its place on the panel, and an interface
// that shows the same controls in a different arrangement is a second thing
// to learn rather than a mirror of the first. So a group carries its
// rectangle in PANEL UNITS -- millimetres of the real hardware -- and the
// drawing scales them to the space it has.
//
// A profile that gives no geometry (every group at the origin with no size)
// falls back to the flowing row. That is deliberate: the geometry of a
// controller nobody here has measured must not be invented, and a wrong
// panel drawing is worse than an honest list.
struct ProfileGroup {
    std::string title;
    std::string accent = "ink";  // "amber", "slate", "sage", "ink", "faint"
    std::vector<std::string> controls;
    int columns = 0;

    float x = 0.0f;  // panel units, from the top-left of the panel
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    // Drawn on the front edge (the face toward the DJ) rather than the top
    // panel. Those controls exist but are not where a top-down picture can
    // put them, so they get their own strip under the panel.
    bool front_panel = false;

    bool placed() const { return w > 0.0f && h > 0.0f; }
};

// A binding shipped with a profile: what a published protocol says the
// control sends. `device` is filled in when the profile is instantiated.
struct ProfileBinding {
    std::string id;
    MidiAddress address;
};

struct DeviceProfile {
    std::string name;          // "reloop_elite": the key settings.json uses
    std::string display_name;  // "Reloop Elite"
    std::string port_hint;     // a fragment of the port's name, e.g. "ELITE"
    // The panel's size in the same units the groups' rectangles use. Zero
    // means this profile carries no geometry and is drawn as a flowing row.
    float panel_w = 0.0f;
    float panel_h = 0.0f;
    // Measured on the hardware, or copied from a document. A profile written
    // from a published protocol stays false until a midi_probe sweep has
    // confirmed it; the interface says so rather than letting a wrong table
    // look like a broken mixer.
    bool verified = false;
    // One line about the device that the control list cannot say, shown under
    // it in TABLE. It exists because a list of identifiers can be complete and
    // still be read wrong: the RP-8000 declares 24 pad ids for EIGHT physical
    // pads, because a layer is a state of the turntable rather than eight more
    // buttons. Nothing in the list says so, and the picture read as 24 buttons
    // to the person who wrote it. Optional, and empty on a device that needs
    // no warning.
    std::string note;
    std::vector<ProfileControl> controls;
    std::vector<ProfileGroup> layout;
    std::vector<ProfileBinding> bindings;
};

// What learn asks for and what the surface declares: the controls, in order.
std::vector<LearnTarget> profile_targets(const DeviceProfile& profile);

// Declares every control on the surface, with its encoder mode, so the whole
// device is drawable as ghosts before anything is bound or touched.
void declare_profile(const DeviceProfile& profile, Surface& surface);

// Installs the shipped bindings for the device at `device_index`.
void bind_profile(const DeviceProfile& profile, std::uint8_t device_index, Surface& surface);

// What is wrong with a profile, one line each; empty means sound. The layout
// may only name declared controls, ids must be unique, bindings must name
// controls -- a typo here would be a knob that draws and never moves.
std::vector<std::string> validate_profile(const DeviceProfile& profile);

const ProfileControl* profile_control(const DeviceProfile& profile, std::string_view id);

// Whether this profile can be drawn as a panel: it has a size and every group
// that is on the top panel has a rectangle.
bool profile_has_geometry(const DeviceProfile& profile);

// Matches a port name fragment against a list of port names: the index of the
// `ordinal`-th port containing the fragment (case-insensitive), or -1. The
// two RP-8000s enumerate as "RP8000mk2" and "2 - RP8000mk2"; both contain the
// fragment, and which one is the left turntable is a fact of the desk.
int match_port(std::string_view fragment, int ordinal, const std::vector<std::string>& ports);

// --- the built-in profiles (profiles_builtin.cpp) ---------------------------
const DeviceProfile& builtin_elite();
const DeviceProfile& builtin_rp8000();  // one turntable; ids carry the deck letter
// The RP-8000 profile for a given deck: "pad.rp8000.a.l1.1" ... three layers.
DeviceProfile rp8000_profile(char deck);
// Every profile compiled in, by name.
std::vector<const DeviceProfile*> builtin_profiles();
const DeviceProfile* builtin_profile(std::string_view name);

}  // namespace svj
