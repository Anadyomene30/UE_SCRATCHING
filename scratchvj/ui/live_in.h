// scratchvj — a live Spout sender as something a hand can scratch.
//
// Deliberately shaped like DeckMedia: poll(), then frame_at(), then a texture
// handle. Downstream of this, a live deck is an ordinary deck -- the 360 pass,
// the taps, the compositor and the effect rack neither know nor need to.
//
// The history lives in MAIN MEMORY, not video memory, and uncompressed. That is
// the honest trade: a cached clip reaches the GPU as BC1 exactly as the .svcache
// stores it, but a live frame has never been analysed, and encoding one to BC1
// per frame is CPU work in the hot path that `core/bc1` is a reference
// implementation for rather than a fast one. So the ring is raw RGBA and its
// depth is bounded by a BYTE BUDGET rather than by a duration -- the same choice
// `core/framewindow` makes, and for the same reason: 720p and 4K then share one
// mechanism instead of needing a special case each.
//
// What the performer gets is therefore a few seconds, not a few minutes, and the
// interface has to show which -- see LiveRing::span_s and `clamped()`.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/livering.h"

namespace svj::ui {

class LiveInput {
public:
    ~LiveInput();

    // `sender` names a Spout sender; empty takes whichever is active. Returns
    // false only when DirectX is unavailable -- NOT when no sender is running,
    // because a sender that has not started yet is the normal case and the
    // receiver is expected to pick it up when it appears.
    bool open(const std::string& sender, std::uint64_t budget_bytes = 256ull << 20);
    void close();
    bool ready() const { return ready_; }

    // Takes whatever has arrived and stamps it `now_s`. Call once a frame.
    // Returns true when a new frame was captured.
    bool poll(double now_s);

    bool connected() const { return connected_; }
    const std::string& sender_name() const { return sender_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

    // The frame nearest `capture_s`, uploaded only when the chosen frame
    // actually changed. Null until something has been received.
    void* frame_at(double capture_s);
    std::uint16_t texture_index() const { return texture_; }

    // How much history is held, and whether the last frame_at() fell outside it.
    // Both are for the interface: a live deck scratched past what was kept must
    // say so rather than showing a frozen picture that reads as a crash.
    const LiveRing& ring() const { return ring_; }
    bool clamped() const { return clamped_; }

    // The stored RGBA of one slot, or null when the slot holds nothing. For
    // tools/live_check, which has to prove the history holds DIFFERENT pictures
    // -- a ring that quietly kept writing the same slot would fill, report a
    // span, and be entirely unscratchable.
    const std::uint8_t* pixels(std::size_t slot) const;

private:
    void resize(std::uint32_t width, std::uint32_t height);

    struct Impl;
    Impl* impl_ = nullptr;

    LiveRing ring_;
    std::vector<std::uint8_t> frames_;  // capacity * width * height * 4
    std::uint64_t budget_bytes_ = 0;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::size_t last_slot_ = static_cast<std::size_t>(-1);
    std::uint16_t texture_ = 0xFFFF;
    bool ready_ = false;
    bool connected_ = false;
    bool clamped_ = false;
    std::string sender_;
};

}  // namespace svj::ui
