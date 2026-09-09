// scratchvj — model of the physical control surface (Reloop Elite, RP-8000 MK2,
// and any controller a profile describes).
//
// This is the "mirror of the table": one entry per physical control, each holding
// its last known value. The knobs on the Elite are ABSOLUTE potentiometers, so at
// launch their real position is unknown until they are first moved. That is not a
// defect to paper over — every control carries a `known` flag and the UI shows
// unknown controls as ghosts rather than inventing a plausible value.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/midi.h"

namespace svj {

enum class ControlKind : std::uint8_t {
    Fader,
    Knob,
    Button,
    Encoder,
    Pad,
};

// How a control's MIDI value is to be read. A potentiometer sends where it IS;
// an endless encoder sends how far it TURNED, in one of two conventions, and
// reading either as a position is the mistake this enum exists to name.
enum class EncoderMode : std::uint8_t {
    Absolute,    // 0..127 is a position
    Relative64,  // 64 is rest; 65.. is clockwise by (v-64), 63.. anticlockwise ("binary offset")
    Signed7,     // two's complement in 7 bits: 1..63 clockwise, 127..65 anticlockwise
};

// Index into a Surface. Stable for the lifetime of the Surface.
using ControlIndex = int;
inline constexpr ControlIndex kNoControl = -1;

struct Control {
    std::string id;  // stable name, e.g. "ch1.eq.hi", "xfader", "pad.elite.a.3"
    ControlKind kind = ControlKind::Knob;
    EncoderMode mode = EncoderMode::Absolute;
    float value = 0.0f;  // always normalised 0..1; bipolar meaning belongs to the mapping
    bool known = false;
    std::uint64_t last_touch_us = 0;
    // Detents turned since last taken, signed. Only a relative encoder fills
    // it: a browse wheel scrolls a list one row per click, which is a count,
    // not a level.
    int ticks = 0;
};

// Identifies the MIDI message that drives a control. Learned at runtime rather
// than hard-coded, because the Elite's CC map is not publicly documented.
struct MidiAddress {
    MidiKind kind = MidiKind::ControlChange;
    std::uint8_t channel = 0;
    std::uint8_t number = 0;
    std::uint8_t device = 0;  // which device of the rig; see MidiEvent::device

    // Note on and note off address the same physical pad.
    static MidiAddress from(const MidiEvent& ev);
    std::uint32_t key() const;
    bool operator==(const MidiAddress& other) const;
};

class Surface {
public:
    // Registers a control. Returns its index; re-declaring an existing id returns
    // the original index and leaves its value untouched (its mode is updated,
    // so a profile can correct an encoder the demo declared as a knob).
    ControlIndex declare(std::string id, ControlKind kind,
                         EncoderMode mode = EncoderMode::Absolute);

    ControlIndex find(std::string_view id) const;
    const Control& at(ControlIndex index) const;
    std::size_t size() const { return controls_.size(); }

    // Binds a MIDI address to a control. One address drives exactly one control;
    // rebinding replaces the previous association.
    void bind(const MidiAddress& address, ControlIndex index);
    ControlIndex bound_to(const MidiAddress& address) const;

    // Routes a MIDI event through the binding table. Returns the control it moved,
    // or kNoControl when the address is unbound. A relative encoder's event
    // moves the value by its delta and counts the detents; everything else
    // sets the value.
    ControlIndex apply(const MidiEvent& event, std::uint64_t now_us);

    // Sets a control directly, marking it known. Used by apply() and by tests.
    void set(ControlIndex index, float value01, std::uint64_t now_us);

    // Detents accumulated on a relative encoder since the last call, and zero
    // them. Zero for anything that is not one.
    int take_ticks(ControlIndex index);

    ControlIndex last_touched() const { return last_touched_; }
    std::size_t unknown_count() const;

    // Drops every known flag without losing bindings or values. Call this when the
    // device reconnects: the values on screen may no longer match the hardware.
    void forget_positions();

private:
    std::vector<Control> controls_;
    std::unordered_map<std::string, ControlIndex> by_id_;
    std::unordered_map<std::uint32_t, ControlIndex> by_address_;
    ControlIndex last_touched_ = kNoControl;
};

// The signed detent count a relative encoder's data byte means, per mode.
// Zero for Absolute. Exposed for the tests and for the learn pass, which has
// to tell a turned encoder from a still one.
int encoder_delta(EncoderMode mode, std::uint16_t value);

}  // namespace svj
