// scratchvj — the settings that describe THIS desk.
//
// Which audio input the platter's carrier arrives on, on which channel pair,
// and what its carrier frequency is. All three were measured on one desk
// (docs/cablage.md: the MOTU, channels 5/6, 1000 Hz) and until now lived as
// constants in the front end. That is the wrong place for a fact about a room:
// a different interface, a cable moved to another pair, a Traktor-configured
// Phase at 2000 Hz -- each would mean a rebuild for something that is
// configuration, not code.
//
// Legible JSON like the mapping and the output presets, and for the same
// reason: a settings file that can be read, diffed and hand-fixed survives its
// own format changing. A missing file is not an error; the defaults are the
// measured desk, so a fresh checkout on the same desk simply works.
#pragma once

#include <string>
#include <string_view>

namespace svj {

struct DeskSettings {
    // A fragment of the capture endpoint's name, matched case-insensitively:
    // "MOTU" finds "In 1-24 (MOTU Pro Audio)".
    std::string platter_endpoint = "MOTU";
    // 0-based index of the pair's left leg: 4 means channels 5/6.
    unsigned platter_first_channel = 4;
    // The carrier at nominal speed. 1000 for the Serato family the Phase
    // emulates; Traktor is 2000, MixVibes 1300. A scale calibration -- wrong
    // by a factor and every velocity is wrong by that factor -- so it is
    // measured with quad_check, never guessed.
    double carrier_hz = 1000.0;
};

std::string settings_to_json(const DeskSettings& settings);

// On failure `error` names the offending field and `out` is left untouched, so
// a broken file cannot half-apply.
bool settings_from_json(std::string_view json, DeskSettings& out, std::string& error);

bool settings_save(const DeskSettings& settings, const std::string& path,
                   std::string& error);

// A missing file loads the defaults and returns true: that is the normal
// first run, not a fault. A file that exists but cannot be parsed is a fault.
bool settings_load(const std::string& path, DeskSettings& out, std::string& error);

}  // namespace svj
