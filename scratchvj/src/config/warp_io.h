// scratchvj — reading and writing output mapping presets.
//
// A projection mapping is aimed at ONE room: this projector, that wall, this
// angle. Getting it right takes a while on a ladder, and losing it because the
// application restarted is not acceptable. So the whole output geometry --
// corner pin, warp mesh, mask -- saves and loads as a named preset, and a venue
// you have played before is a file away rather than an evening away.
//
// Legible JSON on purpose, like config/mapping.json: a preset that can be read,
// diffed and hand-fixed survives its own format changing. Enums are written as
// names rather than numbers for the same reason.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/mesh.h"
#include "core/warp.h"

namespace svj {

struct OutputPreset {
    std::string name;
    CornerPin pin;
    WarpMesh mesh;
    std::vector<Point> mask;
    double mask_feather = 0.0;
    // A preset with the mesh switched off carries it anyway, so turning the
    // grid back on does not lose the work that shaped it.
    bool mesh_enabled = false;
};

std::string preset_to_json(const OutputPreset& preset);

// On failure `error` explains what was wrong, naming the offending field.
bool preset_from_json(std::string_view json, OutputPreset& out, std::string& error);

bool preset_save(const OutputPreset& preset, const std::string& path, std::string& error);
bool preset_load(const std::string& path, OutputPreset& out, std::string& error);

// The .svmap files in a directory, without their extension -- what a picker
// lists. A directory that does not exist is empty rather than an error: the
// first preset creates it.
std::vector<std::string> preset_names(const std::string& directory);

// Where a named preset lives. One place, so the writer and the picker cannot
// disagree about it.
std::string preset_path(const std::string& directory, const std::string& name);

}  // namespace svj
