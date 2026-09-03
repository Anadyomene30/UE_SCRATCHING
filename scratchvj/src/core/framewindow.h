// scratchvj — the resident window of GPU frames.
//
// A whole clip rarely fits in video memory, so a rolling range of frames around
// the playhead is kept resident and refilled from the .svcache in the background.
// Because the cache holds fixed-size block-compressed frames, refilling is a raw
// disk read with no decoding at all.
//
// The window is sized by a VRAM BUDGET, not by a fixed duration. That single
// choice is what lets flat 720p and 4K equirect 360 share one mechanism: the same
// gigabyte holds about 36 seconds of one and about 4 of the other, and neither
// needs a special case.
//
// Anti-thrash rule: the window is recentred only when the playhead APPROACHES AN
// EDGE, never merely because the direction changed. A scratch works back and
// forth in the middle of the window, so it never triggers a refill -- which is
// exactly the case that must never stutter.
#pragma once

#include <cstdint>

namespace svj {

struct FrameRange {
    std::uint32_t first = 0;
    std::uint32_t count = 0;

    bool empty() const { return count == 0; }
    std::uint32_t last() const { return first + count - 1; }
    bool contains(std::uint32_t frame) const {
        return count > 0 && frame >= first && frame <= last();
    }
};

struct WindowConfig {
    // Video memory this deck may hold. Exposed as a setting, because 4K equirect
    // on a modest card needs a smaller window or a smaller analysis resolution.
    std::uint64_t budget_bytes = 1ull << 30;  // 1 GiB

    // Share of the window kept ahead of the playhead when moving forward. Above
    // a half because playback normally goes forwards; a scratch stays central.
    float lead_bias = 0.65f;

    // How close to an edge the playhead may come before the window is recentred.
    std::uint32_t edge_margin = 24;
};

class FrameWindow {
public:
    void configure(std::uint32_t frame_count, std::uint64_t frame_bytes, WindowConfig config);
    void reset();

    std::uint32_t capacity() const { return capacity_; }
    bool holds_whole_clip() const { return capacity_ >= frame_count_ && frame_count_ > 0; }

    // Feeds the playhead and its direction. Returns true when the resident range
    // moved, meaning a refill is due.
    bool update(std::uint32_t playhead, float velocity);

    const FrameRange& resident() const { return resident_; }
    bool contains(std::uint32_t frame) const { return resident_.contains(frame); }

    // What the last update() made newly needed. Empty when nothing moved. After a
    // jump beyond the window this is the whole new range -- the brief load a
    // needle drop outside the window costs.
    const FrameRange& pending_load() const { return pending_; }
    bool reloaded_everything() const { return reloaded_all_; }

    // Duration the window spans, for the bracket drawn on the filmstrip.
    double window_seconds(double frame_duration_s) const;

private:
    FrameRange desired(std::uint32_t playhead, float velocity) const;

    WindowConfig config_;
    std::uint32_t frame_count_ = 0;
    std::uint64_t frame_bytes_ = 0;
    std::uint32_t capacity_ = 0;

    bool placed_ = false;
    FrameRange resident_;
    FrameRange pending_;
    bool reloaded_all_ = false;
};

}  // namespace svj
