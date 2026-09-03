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
    return a;
}

std::uint32_t MidiAddress::key() const {
    return (static_cast<std::uint32_t>(kind) << 16) | (static_cast<std::uint32_t>(channel) << 8) |
           static_cast<std::uint32_t>(number);
}

bool MidiAddress::operator==(const MidiAddress& other) const { return key() == other.key(); }

ControlIndex Surface::declare(std::string id, ControlKind kind) {
    const auto it = by_id_.find(id);
    if (it != by_id_.end()) return it->second;

    const auto index = static_cast<ControlIndex>(controls_.size());
    Control c;
    c.id = id;
    c.kind = kind;
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

ControlIndex Surface::apply(const MidiEvent& event, std::uint64_t now_us) {
    const ControlIndex index = bound_to(MidiAddress::from(event));
    if (index == kNoControl) return kNoControl;

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
