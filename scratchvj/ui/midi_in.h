// scratchvj — a real MIDI input.
//
// The roadmap picked RtMidi for portability, and it will be needed for CoreMIDI
// on the Mac. On Windows the OS library answers the whole question in a hundred
// lines, and `ui/tools/midi_probe` already proved it against the Elite -- so
// this is winmm behind an interface RtMidi can move in behind later, rather
// than a dependency added before it earns its place.
//
// Raw bytes are queued on the callback thread and DECODED on the caller's,
// through the same `core/midi` the tests cover. That split is deliberate: a
// MidiDecoder carries running-status state, and running status only means
// anything within one stream read in order. Decoding on the callback thread and
// draining events would work too, but the state would then live where nothing
// else does.
//
// One port per instance. The rig (ui/midi_rig) holds several: the mixer and
// the two turntables are three ports, and every event carries the index of
// the device it came from so the same CC on two of them stays two addresses.
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "core/midi.h"

namespace svj::ui {

class MidiInput {
public:
    ~MidiInput();

    // Every input port the machine offers, in the order winmm reports them.
    static std::vector<std::string> ports();

    // `port` is a fragment of the name, matched case-insensitively: "ELITE"
    // finds "ELITE". `ordinal` picks the n-th port containing the fragment,
    // which is how the second RP-8000 ("2 - RP8000mk2") is told from the
    // first. `device` is stamped on every event. Returns false when nothing
    // matched, or when another application already holds the port -- which is
    // a FINDING rather than a fault, and exactly what Serato holding the mixer
    // would look like.
    bool open(const std::string& port, int ordinal, std::uint8_t device);
    bool open(const std::string& port) { return open(port, 0, 0); }
    void close();
    // Open and not gone: the driver's close notice (a cable pulled) clears
    // this, so the rig can reopen rather than listen to a dead handle forever.
    bool ready() const { return handle_ != nullptr && !closed_.load(); }
    const std::string& port_name() const { return port_; }
    std::uint8_t device() const { return device_; }

    // Hands over everything decoded since the last call. `out` is replaced.
    void drain(std::vector<MidiEvent>& out);

    // Total messages received, so the interface can show the port is alive even
    // when nothing is bound yet.
    std::uint64_t message_count() const;

    // Called from the driver's callback thread. Public because a C callback has
    // no other way in; not part of the interface anyone else should use.
    void push_raw(std::uint32_t packed);
    void mark_closed() { closed_.store(true); }

private:
    void* handle_ = nullptr;  // HMIDIIN, kept opaque so windows.h stays in the .cpp
    std::string port_;
    std::uint8_t device_ = 0;
    std::atomic<bool> closed_{false};

    // Raw short messages, filled by the callback thread.
    mutable std::mutex lock_;
    std::vector<std::uint8_t> bytes_;
    std::uint64_t messages_ = 0;

    MidiDecoder decoder_;  // caller's thread only
};

}  // namespace svj::ui
