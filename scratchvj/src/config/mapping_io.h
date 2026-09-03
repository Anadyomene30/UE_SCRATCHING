// scratchvj — reading and writing config/mapping.json.
//
// The file holds two things: the MIDI bindings discovered by --midi-learn, and the
// mapping rows that route sources to destinations. It is meant to be legible and
// hand-editable, so every enum is written as a name rather than a number.
//
// This is the only translation unit that includes the JSON library, which keeps
// its compile cost off the rest of the project.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/learn.h"
#include "core/mapping.h"

namespace svj {

struct SurfaceConfig {
    std::vector<LearnResult> bindings;
    std::vector<Mapping> mappings;
};

std::string config_to_json(const SurfaceConfig& config);

// On failure `error` explains what was wrong, naming the offending field.
bool config_from_json(std::string_view json, SurfaceConfig& out, std::string& error);

bool config_save(const SurfaceConfig& config, const std::string& path, std::string& error);
bool config_load(const std::string& path, SurfaceConfig& out, std::string& error);

// Declares every bound control on the surface and installs its MIDI binding.
void config_apply(const SurfaceConfig& config, Surface& surface);

}  // namespace svj
