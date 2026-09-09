// scratchvj — the rig: every MIDI device at once.
//
// The mixer and the two turntables are three ports. One MidiInput per device,
// each stamping its events with the device's index, so the surface's binding
// table -- keyed on device, kind, channel and number -- keeps the mixer's
// CC 7 and a turntable's CC 7 apart.
//
// Reconnection is polled, once a second, by re-enumerating the ports: a DJ
// does not power the rig in the order an application would prefer, and winmm
// does not reliably say when a cable is pulled. A device whose port has gone
// is closed and reopened when it reappears; its knobs are forgotten on the
// way back, because their positions are whatever they physically are.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "config/settings_io.h"
#include "core/midi.h"
#include "core/profile.h"
#include "midi_in.h"

namespace svj::ui {

struct RigDevice {
    DeviceSetting setting;
    const DeviceProfile* profile = nullptr;  // resolved; null when the name is unknown
    std::unique_ptr<MidiInput> input;
    bool was_ready = false;
};

class MidiRig {
public:
    // Builds the devices from the settings. `profiles` resolves names; a
    // device whose profile is unknown is kept, unresolved, and said so.
    void configure(const std::vector<DeviceSetting>& devices,
                   const std::vector<DeviceProfile>& loaded_profiles);

    // Tries to open every device that is not open, at most once a second.
    // Returns the indices of devices that (re)connected on this call, so the
    // caller can forget their knob positions.
    std::vector<std::size_t> poll(double wall_s);

    // Everything decoded since the last call, from every device, device
    // indices stamped. `out` is replaced.
    void drain(std::vector<MidiEvent>& out);

    std::size_t size() const { return devices_.size(); }
    const RigDevice& at(std::size_t index) const { return *devices_[index]; }
    bool connected(std::size_t index) const;
    std::size_t connected_count() const;
    std::uint64_t message_count() const;

    // "Reloop Elite · RP-8000 MK2 A · RP-8000 MK2 B", connected ones only.
    std::string connected_names() const;

private:
    std::vector<std::unique_ptr<RigDevice>> devices_;
    std::vector<MidiEvent> scratch_;
    double last_poll_s_ = -10.0;
};

}  // namespace svj::ui
