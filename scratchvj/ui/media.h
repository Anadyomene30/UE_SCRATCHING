// scratchvj — putting a real frame on screen.
//
// Reads .svcache frames and hands the interface a texture for whatever position
// the deck is at. The path is deliberately unglamorous: read the BC1 frame,
// expand it to RGBA on the CPU (core/bc1), upload. SDL's plain renderer cannot
// take compressed textures, so until bgfx exists this is the stopgap — one
// 0.2-megapixel decode per frame change, which is nothing, and a visible seam
// where bgfx will attach: same bytes, uploaded compressed instead of expanded.
//
// The cache read is also, knowingly, a DISK read per frame rather than a read
// from the VRAM window. The FrameWindow tracks what OUGHT to be resident and the
// engine tests that logic; actually pinning those bytes in memory is the bgfx
// layer's job. Preloading here would just duplicate that machinery in a stopgap.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/videocache.h"

struct SDL_Renderer;
struct SDL_Texture;

namespace svj::ui {

class DeckMedia {
public:
    ~DeckMedia();

    bool open(const std::string& path, std::string& error);
    void close();
    bool ready() const { return open_; }
    const CacheHeader& header() const { return reader_.header(); }

    // The texture for the frame at `position_s`, uploading only when the frame
    // index actually changed. Null when nothing is loaded or a read fails.
    SDL_Texture* frame_at(SDL_Renderer* renderer, double position_s);

    // The last frame decoded by frame_at, as CPU pixels -- what the program
    // compositor consumes. Null until a first frame has been shown.
    const std::uint8_t* pixels() const { return rgba_.empty() ? nullptr : rgba_.data(); }
    std::uint32_t width() const { return reader_.header().width; }
    std::uint32_t height() const { return reader_.header().height; }

private:
    CacheReader reader_;
    SDL_Texture* texture_ = nullptr;
    std::vector<std::uint8_t> packed_;
    std::vector<std::uint8_t> rgba_;
    std::uint32_t last_frame_ = 0xFFFFFFFFu;
    bool open_ = false;
};

}  // namespace svj::ui
