// scratchvj — putting a real frame on screen.
//
// Reads .svcache frames and hands the interface a texture for whatever position
// the deck is at. With bgfx underneath, the BC1 bytes go to the GPU EXACTLY as
// the cache stores them -- no decode, no expansion, which is the entire point
// of analysing clips into a block-compressed format. The hardware has decoded
// BC1 in its samplers since the nineties.
//
// One CPU decode does remain: pixels() feeds the CPU program compositor (the
// tested reference the future GPU compositor will be validated against) and
// the Spout output. It disappears from the hot path the day compositing moves
// onto the GPU; the display path is already free of it.
//
// The cache read is also, knowingly, a DISK read per frame rather than a read
// from the VRAM window. The FrameWindow tracks what OUGHT to be resident and
// the engine tests that logic; actually pinning those bytes is the next step of
// the bgfx layer's job.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/videocache.h"

namespace svj::ui {

class DeckMedia {
public:
    ~DeckMedia();

    bool open(const std::string& path, std::string& error);
    void close();
    bool ready() const { return open_; }
    const CacheHeader& header() const { return reader_.header(); }

    // The texture for the frame at `position_s`, uploaded (compressed) only
    // when the frame index actually changed. Returns an ImTextureID-compatible
    // value; null when nothing is loaded or a read fails.
    void* frame_at(double position_s);

    // The last frame decoded, as CPU pixels -- what the program compositor
    // consumes. Null until a first frame has been shown.
    const std::uint8_t* pixels() const { return rgba_.empty() ? nullptr : rgba_.data(); }
    std::uint32_t width() const { return reader_.header().width; }
    std::uint32_t height() const { return reader_.header().height; }

private:
    CacheReader reader_;
    std::uint16_t texture_ = 0xFFFF;  // bgfx handle index; 0xFFFF = none
    std::vector<std::uint8_t> packed_;
    std::vector<std::uint8_t> rgba_;
    std::uint32_t last_frame_ = 0xFFFFFFFFu;
    bool open_ = false;
};

}  // namespace svj::ui
