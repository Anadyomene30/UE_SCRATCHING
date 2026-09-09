// scratchvj — the library's pictures, on the GPU.
//
// Each analysed clip carries a small BC1 picture in its cache (core/cachemeta).
// The library rows and the pad banks show it, which needs a texture per clip.
// They are made once, kept for the session, and handed to the panels as
// ImTextureIDs indexed by ClipId -- the panel never touches bgfx.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/library.h"

namespace svj::ui {

class ThumbnailStore {
public:
    ~ThumbnailStore();

    // Refreshes the table from the library: a new entry gets its texture, an
    // entry whose cache path changed gets a fresh one, and a clip with no
    // thumbnail yet (still analysing, or an older cache) stays null. Cheap
    // when nothing changed: one comparison per clip.
    void sync(const Library& library);

    // One ImTextureID per ClipId, null where there is no picture. The vector
    // is sized to the library.
    const std::vector<void*>& textures() const { return textures_; }

    void clear();

private:
    struct Slot {
        std::string path;
        std::uint16_t texture = 0xFFFF;
    };
    std::vector<Slot> slots_;
    std::vector<void*> textures_;
};

}  // namespace svj::ui
