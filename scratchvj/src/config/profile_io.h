// scratchvj — reading and writing controller profiles as JSON.
//
// The Elite and the RP-8000 are compiled in (core/profiles_builtin); every
// other controller is a file in profiles/, and this is what reads it. The
// same struct comes out either way, so nothing downstream can tell a table
// from a file. A profile can also be written out -- `scratchvj profile export
// reloop_elite` -- as the template a new one is copied from.
//
// The only translation unit here that includes the JSON library, like the
// rest of config/.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/profile.h"

namespace svj {

std::string profile_to_json(const DeviceProfile& profile);

// On failure `error` names the offending field and `out` is left untouched.
// A profile that parses but does not validate (core/profile.h) fails too,
// with the first fault as the reason.
bool profile_from_json(std::string_view json, DeviceProfile& out, std::string& error);

bool profile_save(const DeviceProfile& profile, const std::string& path, std::string& error);
bool profile_load(const std::string& path, DeviceProfile& out, std::string& error);

// Every *.json in `directory`, loaded; files that fail are reported in
// `errors` (one line each) and skipped rather than sinking the rest.
std::vector<DeviceProfile> profiles_in(const std::string& directory,
                                       std::vector<std::string>& errors);

// A profile by name: a built-in one, or one of `loaded`. Null when unknown.
const DeviceProfile* find_profile(std::string_view name,
                                  const std::vector<DeviceProfile>& loaded);

}  // namespace svj
