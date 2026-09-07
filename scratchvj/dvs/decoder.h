// scratchvj — the DVS timecode decoder, wrapping xwax's timecoder.
//
// This is a separate layer from `core/` on purpose, for two reasons that both
// matter:
//
//   1. `core/` has no external dependency, and that is what lets it be tested on
//      three operating systems from a bare checkout. xwax's timecoder is
//      vendored C; putting it in core would end that guarantee.
//   2. xwax is GPL-3. Keeping it in its own layer keeps the boundary visible
//      rather than implicit -- the same reason `unreal/ScratchLink` links to
//      nothing of ours.
//
// What comes out is a `DecoderSample`, which `core/timecode` has consumed since
// the first commit and which its tests already cover. So this file is a bridge
// and nothing more: no policy, no smoothing, no interpretation. Everything about
// what a position MEANS -- lock loss, jumps, the wireless profile, ABS/REL --
// already lives in core and stays there.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/timecode.h"

namespace svj::dvs {

// The timecode formats xwax knows, by its own names. `serato_2a` is side A of
// the Serato control vinyl, which is what an MWM Phase emulates -- but WHICH of
// the Serato definitions actually locks is measured by tools/dvs_check rather
// than assumed here.
std::vector<std::string> known_definitions();

class TimecodeDecoder {
public:
    TimecodeDecoder();
    ~TimecodeDecoder();

    TimecodeDecoder(const TimecodeDecoder&) = delete;
    TimecodeDecoder& operator=(const TimecodeDecoder&) = delete;

    // `definition` is one of known_definitions(). Returns false on an unknown
    // name or an implausible sample rate.
    bool open(const std::string& definition, unsigned sample_rate);
    void close();
    bool ready() const { return impl_ != nullptr; }
    const std::string& definition() const { return definition_; }

    // Feeds one block of INTERLEAVED STEREO samples and reports what the
    // decoder now knows. `now_s` stamps the block for core/timecode's clock.
    //
    // The two channels are the two sides of the quadrature pair, and their
    // ORDER decides which way the record appears to turn. Swapping them inverts
    // every direction downstream, so it is a wiring fact to be calibrated, not
    // guessed -- see tools/dvs_check.
    DecoderSample submit(const std::int16_t* interleaved, std::size_t frames,
                         double now_s);
    // Same, for a float source such as WASAPI's mix format. Samples outside
    // [-1, 1] are clipped rather than wrapped: a wrapped sample would look like
    // a violent direction reversal to the phase detector.
    DecoderSample submit(const float* interleaved, std::size_t frames, double now_s);

    // Carrier level of the last block, 0..1. Computed here rather than taken
    // from xwax, which exposes no such reading: it is the plain peak of what was
    // fed in, and it is what tells "the needle is up" from "the decoder is
    // confused".
    float level() const { return level_; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    std::string definition_;
    unsigned sample_rate_ = 0;
    float level_ = 0.0f;
    std::vector<std::int16_t> scratch_;  // float conversion buffer
};

}  // namespace svj::dvs
