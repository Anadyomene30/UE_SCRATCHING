// scratchvj — a scripted performance, for running the engine without turntables.
//
// Everything downstream of the decoder is exercised by this: the timecode tracker
// and its wireless profile, gesture detection, the transport, the mapping engine
// and the VRAM window. The script deliberately includes the awkward moments --
// a backspin, a loop scratched inside, a link dropout -- because those are the
// ones worth watching, and the ones a hand-waved demo always leaves out.
//
// It is deterministic: the same second of the script always produces the same
// numbers, so a change in behaviour is a change in the code and not in the noise.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/surface.h"
#include "core/timecode.h"

namespace svj {

struct SimEvent {
    bool loop_in = false;
    bool loop_exit = false;
    bool cue_jump = false;
    int cue_index = 0;
    bool slip_on = false;
    bool slip_off = false;
};

class Simulation {
public:
    // Declares the controls it will move, so the surface mirrors a real table.
    void configure(Surface& surface);

    double length_s() const { return 24.0; }

    // Advances to `t` seconds into the script (it repeats after length_s()).
    void step(double t_s, Surface& surface, std::uint64_t now_us);

    DecoderSample deck_a() const { return deck_a_; }
    DecoderSample deck_b() const { return deck_b_; }
    const SimEvent& events() const { return events_; }

    // A one-line description of what the script is doing right now.
    const std::string& phase() const { return phase_; }

    // The sound the script is notionally playing, for the reactive bands: a kick
    // on every beat, a hat on every off-beat, and a bass note under both, at the
    // BPM passed in. Synthetic on purpose -- the point is to exercise the
    // analyser and everything mapped to it without a sound card, exactly as the
    // rest of this file exercises the engine without turntables.
    //
    // `from_s` to `to_s` is the span to render; the phase is derived from the
    // absolute time, so the result does not depend on how the caller chops it
    // up. Writes into `out` and returns how many samples it wrote.
    std::size_t audio(double from_s, double to_s, double bpm, double sample_rate,
                      float* out, std::size_t capacity) const;

private:
    DecoderSample deck_a_;
    DecoderSample deck_b_;
    SimEvent events_;
    std::string phase_;
    double previous_t_ = -1.0;

    ControlIndex xfader_ = kNoControl;
    ControlIndex hi_ = kNoControl;
    ControlIndex mid_ = kNoControl;
    ControlIndex filter_ = kNoControl;
    ControlIndex fader1_ = kNoControl;
    ControlIndex fader2_ = kNoControl;
};

}  // namespace svj
