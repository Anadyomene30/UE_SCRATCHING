#include "core/surface.h"

#include <algorithm>
#include <stdexcept>

namespace svj {

MidiAddress MidiAddress::from(const MidiEvent& ev) {
    MidiAddress a;
    // A pad press and its release are the same physical control, so note off is
    // folded onto note on rather than being addressed separately.
    a.kind = ev.kind == MidiKind::NoteOff ? MidiKind::NoteOn : ev.kind;
    a.channel = ev.channel;
    a.number = ev.kind == MidiKind::PitchBend ? 0 : ev.number;
    a.device = ev.device;
    return a;
}

std::uint32_t MidiAddress::key() const {
    // The device in the top byte: the same CC on two devices is two addresses.
    return (static_cast<std::uint32_t>(device) << 24) | (static_cast<std::uint32_t>(kind) << 16) |
           (static_cast<std::uint32_t>(channel) << 8) | static_cast<std::uint32_t>(number);
}

bool MidiAddress::operator==(const MidiAddress& other) const { return key() == other.key(); }

int encoder_delta(EncoderMode mode, std::uint16_t value) {
    const int v = static_cast<int>(value & 0x7F);
    switch (mode) {
        case EncoderMode::Relative64: return v - 64;
        case EncoderMode::Signed7: return v < 64 ? v : v - 128;
        case EncoderMode::Absolute:
        default: return 0;
    }
}

ControlIndex Surface::declare(std::string id, ControlKind kind, EncoderMode mode) {
    const auto it = by_id_.find(id);
    if (it != by_id_.end()) {
        controls_[static_cast<std::size_t>(it->second)].mode = mode;
        return it->second;
    }

    const auto index = static_cast<ControlIndex>(controls_.size());
    Control c;
    c.id = id;
    c.kind = kind;
    c.mode = mode;
    controls_.push_back(std::move(c));
    by_id_.emplace(std::move(id), index);
    return index;
}

ControlIndex Surface::find(std::string_view id) const {
    const auto it = by_id_.find(std::string(id));
    return it == by_id_.end() ? kNoControl : it->second;
}

const Control& Surface::at(ControlIndex index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= controls_.size()) {
        throw std::out_of_range("svj::Surface::at: control index out of range");
    }
    return controls_[static_cast<std::size_t>(index)];
}

void Surface::bind(const MidiAddress& address, ControlIndex index) {
    by_address_[address.key()] = index;
}

ControlIndex Surface::bound_to(const MidiAddress& address) const {
    const auto it = by_address_.find(address.key());
    return it == by_address_.end() ? kNoControl : it->second;
}

void Surface::set(ControlIndex index, float value01, std::uint64_t now_us) {
    if (index < 0 || static_cast<std::size_t>(index) >= controls_.size()) return;
    Control& c = controls_[static_cast<std::size_t>(index)];
    c.value = std::clamp(value01, 0.0f, 1.0f);
    c.known = true;
    c.last_touch_us = now_us;
    last_touched_ = index;
}

int Surface::take_ticks(ControlIndex index) {
    if (index < 0 || static_cast<std::size_t>(index) >= controls_.size()) return 0;
    Control& c = controls_[static_cast<std::size_t>(index)];
    const int ticks = c.ticks;
    c.ticks = 0;
    return ticks;
}

ControlIndex Surface::apply(const MidiEvent& event, std::uint64_t now_us) {
    const ControlIndex index = bound_to(MidiAddress::from(event));
    if (index == kNoControl) return kNoControl;

    Control& c = controls_[static_cast<std::size_t>(index)];
    if (c.mode != EncoderMode::Absolute && event.kind == MidiKind::ControlChange) {
        // A relative encoder: the byte is a distance, not a place. The value
        // walks by a fixed step per detent -- 32 detents end to end, which is
        // about what a full turn of a browse wheel covers -- and the detents
        // are counted for whoever scrolls a list by them.
        const int delta = encoder_delta(c.mode, event.value);
        if (delta == 0) return index;  // rest, or an idle report on connect
        c.ticks += delta;
        set(index, c.value + static_cast<float>(delta) / 32.0f, now_us);
        return index;
    }

    // A note off releases the pad; everything else carries its own value.
    const float value = event.kind == MidiKind::NoteOff ? 0.0f : event.normalised();
    set(index, value, now_us);
    return index;
}

std::size_t Surface::unknown_count() const {
    return static_cast<std::size_t>(
        std::count_if(controls_.begin(), controls_.end(), [](const Control& c) { return !c.known; }));
}

void Surface::forget_positions() {
    for (Control& c : controls_) {
        c.known = false;
        c.last_touch_us = 0;
    }
    last_touched_ = kNoControl;
}

}  // namespace svj
