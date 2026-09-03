// scratchvj — the default control layout of the Reloop Elite and RP-8000 MK2.
//
// This is a CHECKLIST, not a driver. It names every control the user will be
// asked to sweep during --midi-learn, and fixes the order in which state packets
// carry them. It deliberately holds no CC numbers: the Elite's MIDI map is not
// publicly documented, so the numbers are discovered at runtime and any other
// mixer works by learning against the same checklist.
#pragma once

#include <string>
#include <vector>

#include "core/learn.h"
#include "core/surface.h"

namespace svj {

// The mixer alone: channel strips, crossfader, effect section, pads, browse.
std::vector<LearnTarget> elite_layout();

// One turntable's performance pads. `deck` is 'a' or 'b'; `layer` selects which
// of the three pad layers the ids belong to.
std::vector<LearnTarget> rp8000_layout(char deck, int layer = 1);

// The full default rig: the Elite plus both turntables on their first layer.
std::vector<LearnTarget> default_rig_layout();

// Declares every target on a surface without binding anything, so the UI can draw
// the whole table as ghosts before a single control has been learned or touched.
void declare_layout(const std::vector<LearnTarget>& targets, Surface& surface);

}  // namespace svj
