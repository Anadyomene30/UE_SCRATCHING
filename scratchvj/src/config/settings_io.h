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
#include <vector>

#include "core/mixer.h"

namespace svj {

// One MIDI device of the rig: which profile describes it, which port it is
// on, and -- for two identical devices, the pair of RP-8000s -- which of the
// matching ports. `deck` is the letter a turntable's pads are named after.
struct DeviceSetting {
    std::string profile = "reloop_elite";
    std::string port = "ELITE";  // a fragment of the port's name
    int ordinal = 0;             // the n-th port containing the fragment
    char deck = 'a';             // for profiles whose ids carry a deck letter
};

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

    // A fragment of the MIDI input port's name. "ELITE" on this desk; the
    // mixer's control map is never hard-coded, only which port to listen to.
    // Kept for files written before `devices` existed: when `devices` is
    // empty, the rig is this one port with the Elite's profile.
    std::string midi_port = "ELITE";

    // The rig: every MIDI device, in the order their indices are numbered.
    // The default is the measured desk -- the mixer and the two turntables,
    // which Windows lists as "RP8000mk2" and "2 - RP8000mk2".
    std::vector<DeviceSetting> devices{
        DeviceSetting{"reloop_elite", "ELITE", 0, 'a'},
        DeviceSetting{"rp8000", "RP8000", 0, 'a'},
        DeviceSetting{"rp8000", "RP8000", 1, 'b'},
    };

    // The mixer's switches, as the performer set them.
    MixSettings mix;

    // The screen the program goes to, by NAME rather than by index: display
    // indices are renumbered whenever something is plugged in, and a set that
    // opens on the wrong screen because a hub enumerated differently is worse
    // than one that opens on none. Empty means no output screen.
    std::string output_display;
    // Whether it was open when the application last closed, so a rig that is
    // always the same rig comes back the way it was left.
    bool output_open = false;

    // Where the rushes are. Walked recursively at startup and on request;
    // `clips/` next to the working directory is where the analysis pass used
    // to leave things, so it stays the default rather than orphaning them.
    std::vector<std::string> library_folders{"clips"};
    // Where caches go. Empty means next to their source, which is right for a
    // folder the performer owns and wrong for a read-only drive.
    std::string cache_dir;
    // The cadence a numbered image sequence is given: the files do not carry
    // one, and a pass has to pick something.
    double sequence_fps = 30.0;
    // Frames are downscaled to at most this wide when analysed.
    unsigned analysis_max_width = 1024;

    // Video memory each deck may hold, in MiB. 256 shows the window slide on
    // a short clip; a card with room can take a gigabyte and hold a whole
    // 4K equirect.
    unsigned vram_budget_mb = 256;

    // Where the control stream goes: the Unreal machine, or this one.
    std::string control_host = "127.0.0.1";
    unsigned control_port = 7331;
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
