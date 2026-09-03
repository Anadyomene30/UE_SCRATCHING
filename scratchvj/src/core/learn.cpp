#include "core/learn.h"

#include <algorithm>

namespace svj {

void MidiLearn::begin(std::vector<LearnTarget> targets) {
    targets_ = std::move(targets);
    index_ = 0;
    observations_.clear();
    results_.clear();
}

void MidiLearn::advance() {
    ++index_;
    observations_.clear();
}

void MidiLearn::skip() {
    if (active()) advance();
}

bool MidiLearn::address_taken(const MidiAddress& address) const {
    const std::uint32_t key = address.key();
    return std::any_of(results_.begin(), results_.end(),
                       [key](const LearnResult& r) { return r.address.key() == key; });
}

bool MidiLearn::observe(const MidiEvent& event) {
    if (!active()) return false;

    // A note off is the release of a pad already counted on its press.
    if (event.kind == MidiKind::NoteOff) return false;

    const MidiAddress address = MidiAddress::from(event);

    // Refuse an address already claimed earlier in this session, otherwise a knob
    // still settling would steal the binding of the control learned before it.
    if (address_taken(address)) return false;

    const LearnTarget& target = current();
    const bool momentary =
        target.kind == ControlKind::Button || target.kind == ControlKind::Pad;

    if (momentary) {
        if (event.kind != MidiKind::NoteOn && event.kind != MidiKind::ControlChange) return false;
        // A pad only counts on its press, not on a zero-valued release.
        if (event.value == 0) return false;
        results_.push_back(LearnResult{target.id, target.kind, address});
        advance();
        return true;
    }

    Observation& observation = observations_[address.key()];
    if (!observation.seen) {
        observation.seen = true;
        observation.last_value = event.value;
        observation.distinct_moves = 1;
    } else if (event.value != observation.last_value) {
        observation.last_value = event.value;
        ++observation.distinct_moves;
    }

    if (observation.distinct_moves < required_moves_) return false;

    results_.push_back(LearnResult{target.id, target.kind, address});
    advance();
    return true;
}

void MidiLearn::apply_to(Surface& surface) const {
    for (const LearnResult& r : results_) {
        const ControlIndex index = surface.declare(r.id, r.kind);
        surface.bind(r.address, index);
    }
}

}  // namespace svj
