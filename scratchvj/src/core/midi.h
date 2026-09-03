// scratchvj — MIDI message decoding.
//
// Pure logic: takes raw MIDI bytes and produces normalised events. Knows nothing
// about devices or drivers, so it is fully unit-testable on any platform.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace svj {

enum class MidiKind : std::uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    PitchBend,
    Other,
};

struct MidiEvent {
    MidiKind kind = MidiKind::Other;
    std::uint8_t channel = 0;  // 0-15
    std::uint8_t number = 0;   // note number, or CC number (ignored for pitch bend)
    std::uint16_t value = 0;   // 0-127, or 0-16383 for pitch bend and 14-bit CC
    bool high_resolution = false;

    // Value scaled to 0..1 regardless of 7-bit or 14-bit resolution.
    float normalised() const;
};

// Decodes a MIDI byte stream, handling running status and reassembling 14-bit CC
// pairs (controller n as MSB, controller n+32 as LSB, per the MIDI spec).
//
// An MSB is emitted immediately as a 7-bit event so nothing is delayed waiting on
// an LSB that may never come. If the matching LSB does arrive next, a second event
// carrying the full 14-bit value follows it, and consumers simply take the later
// value. Controllers that only ever send MSBs therefore work unchanged.
class MidiDecoder {
public:
    // Feeds raw bytes and appends any decoded events to `out`.
    void feed(const std::uint8_t* bytes, std::size_t count, std::vector<MidiEvent>& out);

    void reset();

private:
    void dispatch(std::vector<MidiEvent>& out);

    std::uint8_t status_ = 0;            // running status, 0 when none is held
    std::uint8_t data_[2] = {0, 0};
    std::uint8_t data_count_ = 0;
    std::uint8_t expected_data_ = 0;

    // Pending 14-bit CC MSB, awaiting a possible LSB on controller number + 32.
    bool have_pending_msb_ = false;
    std::uint8_t pending_channel_ = 0;
    std::uint8_t pending_number_ = 0;
    std::uint8_t pending_msb_ = 0;
};

}  // namespace svj
