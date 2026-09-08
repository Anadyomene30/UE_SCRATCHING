#include "midi_rig.h"

#include "config/profile_io.h"

namespace svj::ui {

void MidiRig::configure(const std::vector<DeviceSetting>& devices,
                        const std::vector<DeviceProfile>& loaded_profiles) {
    devices_.clear();
    for (const DeviceSetting& setting : devices) {
        auto device = std::make_unique<RigDevice>();
        device->setting = setting;
        device->profile = find_profile(setting.profile, loaded_profiles);
        device->input = std::make_unique<MidiInput>();
        devices_.push_back(std::move(device));
    }
    last_poll_s_ = -10.0;
}

std::vector<std::size_t> MidiRig::poll(double wall_s) {
    std::vector<std::size_t> fresh;
    if (wall_s - last_poll_s_ < 1.0) return fresh;
    last_poll_s_ = wall_s;

    // One enumeration for every device: winmm's device list is what says
    // whether a port that was open still exists.
    const std::vector<std::string> ports = MidiInput::ports();
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        RigDevice& device = *devices_[i];
        const int found = match_port(device.setting.port, device.setting.ordinal, ports);
        if (device.input->ready()) {
            if (found < 0 || ports[static_cast<std::size_t>(found)] != device.input->port_name()) {
                // The port is gone or has moved: close, and let the next
                // poll reopen it.
                device.input->close();
                device.was_ready = false;
            }
            continue;
        }
        if (device.input->port_name().size() > 0) device.input->close();  // closed by the driver
        if (found < 0) continue;
        if (device.input->open(device.setting.port, device.setting.ordinal,
                               static_cast<std::uint8_t>(i))) {
            fresh.push_back(i);
            device.was_ready = true;
        }
    }
    return fresh;
}

void MidiRig::drain(std::vector<MidiEvent>& out) {
    out.clear();
    for (const auto& device : devices_) {
        if (!device->input->ready()) continue;
        device->input->drain(scratch_);
        out.insert(out.end(), scratch_.begin(), scratch_.end());
    }
}

bool MidiRig::connected(std::size_t index) const {
    return index < devices_.size() && devices_[index]->input->ready();
}

std::size_t MidiRig::connected_count() const {
    std::size_t n = 0;
    for (const auto& device : devices_) {
        if (device->input->ready()) ++n;
    }
    return n;
}

std::uint64_t MidiRig::message_count() const {
    std::uint64_t total = 0;
    for (const auto& device : devices_) total += device->input->message_count();
    return total;
}

std::string MidiRig::connected_names() const {
    std::string names;
    for (const auto& device : devices_) {
        if (!device->input->ready()) continue;
        if (!names.empty()) names += " \xC2\xB7 ";
        names += device->profile != nullptr ? device->profile->display_name
                                            : device->setting.profile;
        if (device->profile != nullptr && device->profile->name == "rp8000") {
            names += device->setting.deck == 'b' ? " B" : " A";
        }
    }
    return names;
}

}  // namespace svj::ui
