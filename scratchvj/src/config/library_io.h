// scratchvj — reading and writing library.json.
//
// What the scan cannot rediscover on its own: which caches came from which
// sources, the projection the performer forced on a clip, the crates, and
// (later) the clip matrix. The clips themselves are found again by walking
// the folders; this file is the memory of what was DECIDED about them.
//
// Legible JSON like the other config files, and for the same reason: a file
// that can be read, diffed and hand-fixed survives its own format changing.
// The only translation unit here that includes the JSON library, like the
// rest of config/.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/library.h"
#include "core/matrix.h"

namespace svj {

struct LibraryFile {
    struct Clip {
        std::string source;   // empty for an orphan cache
        std::string cache;
        std::string name;
        ProjectionOverride projection = ProjectionOverride::Auto;
        bool is_sequence = false;
        bool is_still = false;
    };
    struct Crate {
        std::string name;
        std::vector<std::string> clips;  // by cache path: the one key every entry has
    };
    // A pad bank: eight cache paths per layer, "" for an empty pad.
    struct Bank {
        std::string name;
        std::vector<std::string> a;
        std::vector<std::string> b;
        std::vector<std::string> overlay;
    };
    std::vector<Clip> clips;
    std::vector<Crate> crates;
    std::vector<Bank> banks;
    int current_bank = 0;
};

std::string library_to_json(const LibraryFile& file);

// On failure `error` names the offending field and `out` is left untouched.
bool library_from_json(std::string_view json, LibraryFile& out, std::string& error);

bool library_save(const LibraryFile& file, const std::string& path, std::string& error);

// A missing file is an empty library and returns true: the normal first run.
bool library_load(const std::string& path, LibraryFile& out, std::string& error);

// The two directions between the file and the live Library. Pure: the
// snapshot reads, the restore adds entries as Unanalysed with the cache path
// as their `path`; whether each cache actually exists is the front end's
// question, answered with a disk.
LibraryFile library_snapshot(const Library& library);
void library_restore(const LibraryFile& file, Library& library);
// With the pad banks. A bank cell naming a path the library does not hold
// is restored empty: a pad must never load a row that does not exist.
LibraryFile library_snapshot(const Library& library, const Matrix& matrix);
void library_restore(const LibraryFile& file, Library& library, Matrix& matrix);

}  // namespace svj
