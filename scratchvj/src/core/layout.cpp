#include "core/layout.h"

#include "core/profile.h"

namespace svj {

// The checklists are the profiles' rosters: one description of a device, used
// for learning, for the wire schema and for the drawing alike.

std::vector<LearnTarget> elite_layout() { return profile_targets(builtin_elite()); }

std::vector<LearnTarget> rp8000_layout(char deck, int layer) {
    std::vector<LearnTarget> targets;
    const std::string prefix =
        std::string("pad.rp8000.") + deck + ".l" + std::to_string(layer) + ".";
    for (const LearnTarget& t : profile_targets(rp8000_profile(deck))) {
        if (t.id.rfind(prefix, 0) == 0) targets.push_back(t);
    }
    return targets;
}

std::vector<LearnTarget> default_rig_layout() {
    std::vector<LearnTarget> targets = elite_layout();
    // Both turntables, all three pad layers: a layer that is not declared is a
    // layer whose pads can never be learned.
    for (const char deck : {'a', 'b'}) {
        const std::vector<LearnTarget> pads = profile_targets(rp8000_profile(deck));
        targets.insert(targets.end(), pads.begin(), pads.end());
    }
    return targets;
}

void declare_layout(const std::vector<LearnTarget>& targets, Surface& surface) {
    for (const LearnTarget& target : targets) {
        surface.declare(target.id, target.kind);
    }
}

}  // namespace svj
