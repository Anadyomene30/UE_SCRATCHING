// scratchvj — MIDI learn.
//
// The Reloop Elite's CC map is not publicly documented, so nothing is hard-coded:
// the user sweeps each control once and the binding is written out. The useful
// side effect is that the whole project works with any other mixer.
//
// Binding on the first event that arrives would be fragile -- controllers emit
// stray traffic, and a neighbouring knob brushed in passing would win. Instead a
// continuous control must produce several DISTINCT values before it is accepted,
// while a button or pad binds on a single press.
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/midi.h"
#include "core/surface.h"

namespace svj {

struct LearnTarget {
    std::string id;
    ControlKind kind = ControlKind::Knob;
};

struct LearnResult {
    std::string id;
    ControlKind kind = ControlKind::Knob;
    MidiAddress address;
};

class MidiLearn {
public:
    // Distinct values a continuous control must emit before it is accepted.
    explicit MidiLearn(int required_moves = 6) : required_moves_(required_moves) {}

    void begin(std::vector<LearnTarget> targets);

    bool active() const { return index_ < targets_.size(); }
    const LearnTarget& current() const { return targets_[index_]; }
    std::size_t remaining() const { return targets_.size() - index_; }

    // Feeds one event. Returns true when it completed the current target, at which
    // point current() has advanced to the next one.
    bool observe(const MidiEvent& event);

    // Abandons the current target and moves on, leaving it unbound.
    void skip();

    const std::vector<LearnResult>& results() const { return results_; }

    // Applies every learned binding to a surface, declaring controls as needed.
    void apply_to(Surface& surface) const;

private:
    struct Observation {
        int distinct_moves = 0;
        std::uint16_t last_value = 0;
        bool seen = false;
    };

    void advance();
    bool address_taken(const MidiAddress& address) const;

    int required_moves_;
    std::vector<LearnTarget> targets_;
    std::size_t index_ = 0;
    std::unordered_map<std::uint32_t, Observation> observations_;
    std::vector<LearnResult> results_;
};

}  // namespace svj
