// scratchvj — putting a real frame on screen.
//
// Reads .svcache frames and hands the interface a texture for whatever position
// the deck is at. With bgfx underneath, the BC1 bytes go to the GPU EXACTLY as
// the cache stores them -- no decode, no expansion, which is the entire point
// of analysing clips into a block-compressed format. The hardware has decoded
// BC1 in its samplers since the nineties.
//
// No CPU decode remains in the hot path at all: the program compositor runs on
// the GPU as well (ui/gpu_compose), and Spout is fed by readback of its output.
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
#include "core/videotaps.h"

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

    std::uint32_t width() const { return reader_.header().width; }
    std::uint32_t height() const { return reader_.header().height; }
    bool has_alpha() const { return reader_.header().has_alpha(); }

    // The bgfx handle index of the deck's texture, for the GPU compositor.
    // 0xFFFF until a first frame has been shown.
    std::uint16_t texture_index() const { return texture_; }

    // Uploads the clip at every position `plan` names into a texture ARRAY --
    // one layer per moment -- and returns its handle index. This is what makes
    // trails and slit scan possible: the clip at eight moments at once, each
    // fetched by position rather than remembered from a previous frame.
    //
    // A layer whose frame has not changed is not re-uploaded, so a collapsed
    // plan (a stopped record) costs one read rather than eight, and a slow
    // scratch costs only the layers that actually crossed a frame boundary.
    std::uint16_t taps_texture(const TapPlan& plan);

    // The same texture as an ImTextureID, for panels that show the SOURCE
    // rather than the projected view -- the 360 layout's sight frame.
    void* imgui_texture() const {
        return texture_ == 0xFFFF
                   ? nullptr
                   : reinterpret_cast<void*>(static_cast<std::uint64_t>(texture_) + 1);
    }

private:
    CacheReader reader_;
    std::uint16_t texture_ = 0xFFFF;  // bgfx handle index; 0xFFFF = none
    std::uint16_t taps_ = 0xFFFF;     // the layered texture, created on demand
    std::uint32_t tap_frames_[kTapCount] = {};
    std::vector<std::uint8_t> packed_;
    std::uint32_t last_frame_ = 0xFFFFFFFFu;
    bool open_ = false;
};

}  // namespace svj::ui
