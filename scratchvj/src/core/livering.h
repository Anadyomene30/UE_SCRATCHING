// scratchvj — making a live stream into something a hand can scratch.
//
// A camera, a Spout sender or an NDI feed has no timeline: it has a present, and
// the present cannot be rewound. That is exactly the shape design principle 1
// forbids, so a live source cannot simply become a deck.
//
// WHAT IT CAN BECOME is the last few seconds of itself. Those DO have a
// timeline. This is a ring of the most recent frames, addressed by their CAPTURE
// TIME, and reading it stays a pure function of position -- the ring's content
// advances with the wall clock because that is what "live" means, but nothing
// about looking into it accumulates. Scratching the last four seconds of a
// camera is a turntablist idea rather than a compromise, and it is the honest
// one: the machine can only scratch what it has actually kept.
//
// THE CHOICE THAT DECIDES THE FEEL. Holding a position while frames keep
// arriving can mean two things, and they are different instruments:
//
//   - hold the AGE: stay two seconds behind, and the picture keeps moving. That
//     is a delay line.
//   - hold the FRAME: stay on the picture you grabbed, and let its age grow
//     until it falls off the tail of the ring. That is a record.
//
// A deck is a record, so `pick` addresses capture time and the caller holds a
// capture time rather than an age. A delay wanting the other behaviour asks for
// `newest_s() - age` each frame, which is one line and says what it means.
//
// This file stores TIMES AND SLOTS, never pixels: the frames live in GPU
// textures the front end owns, and `core/` keeps its rule of having no
// dependency. push() hands back the slot to write into.
#pragma once

#include <cstddef>
#include <vector>

namespace svj {

// Which frame answers a request, and whether the request could be answered.
struct LivePick {
    std::size_t slot = 0;
    double capture_s = 0.0;  // the capture time of the frame actually chosen
    bool valid = false;      // false only when the ring is empty
    // True when the request fell outside what the ring holds. The picture is
    // still the nearest one, but the caller is told rather than being handed a
    // frozen frame that looks like a fault. Scratching past the tail of a live
    // capture is a real limit and the interface has to be able to show it.
    bool clamped = false;
};

class LiveRing {
public:
    // `capacity` frames of history. The duration that buys depends on how fast
    // the source actually delivers, which is why nothing here takes a frame
    // rate: a live source drops frames, and any arithmetic assuming it does not
    // picks the wrong frame precisely when the source is struggling.
    void configure(std::size_t capacity);
    void reset();

    // Records a frame captured at `capture_s` and returns the slot the caller
    // must write its pixels into. Frames arriving out of order, or with a time
    // that has jumped backwards -- a source restarting, a clock resetting -- are
    // taken as a new beginning and clear the ring, because a ring whose times do
    // not increase cannot be searched.
    std::size_t push(double capture_s);

    std::size_t capacity() const { return capacity_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // The capture times at the two ends. `newest_s()` is the present.
    double newest_s() const;
    double oldest_s() const;
    // How much history is actually available, which is what an interface should
    // draw: it grows as the ring fills and then stops.
    double span_s() const;

    // The frame nearest `capture_s`. Nearest by TIME, not by index arithmetic:
    // frames do not arrive on a grid.
    LivePick pick(double capture_s) const;

private:
    // Ring storage of capture times; `head_` is where the next push lands.
    std::vector<double> times_;
    std::size_t capacity_ = 0;
    std::size_t head_ = 0;
    std::size_t size_ = 0;

    // Slot of the i-th oldest frame.
    std::size_t slot_of(std::size_t age_index) const;
};

}  // namespace svj
