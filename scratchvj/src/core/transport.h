// scratchvj — loops, hot cues, beat jump and slip.
//
// This is the layer that turns a platter position into a PLAYED position. Without
// it the instrument is a demo: a DJ with no cue points and no loops is amputated,
// and no amount of video cleverness makes up for it.
//
// Everything works by OFFSET rather than by moving a playhead. A hot cue, a beat
// jump and a loop wrap all just change the offset between where the platter is and
// where the clip plays. That keeps the whole thing a pure function of position,
// which is the rule the rest of the engine is built on -- so all of it stays
// scratchable, loops included.
#pragma once

#include <array>
#include <cstdint>

namespace svj {

inline constexpr int kHotCueCount = 8;

struct HotCue {
    double position_s = 0.0;
    bool set = false;
    std::uint8_t colour = 0;
};

struct Loop {
    double start_s = 0.0;
    double end_s = 0.0;
    bool active = false;

    double length_s() const { return end_s - start_s; }
};

struct BeatGrid {
    double bpm = 0.0;
    double first_beat_s = 0.0;

    bool valid() const { return bpm > 0.0; }
    double beat_duration_s() const { return valid() ? 60.0 / bpm : 0.0; }

    // Nearest beat boundary to a time. Used to quantise loops and jumps so they
    // land musically rather than wherever the hand happened to be.
    double snap(double seconds) const;
};

class Transport {
public:
    void configure(double clip_duration_s, BeatGrid grid);
    void reset();

    // Feeds the position the platter is asking for; returns the position actually
    // played. Call once per block, in order.
    double map(double platter_position_s);

    // ---- loops ----
    void loop_in(double clip_position_s);
    void loop_out(double clip_position_s);
    // Sets a loop of `beats` beats starting at the current played position.
    void auto_loop(double beats);
    void exit_loop();
    const Loop& loop() const { return loop_; }

    // ---- hot cues ----
    void set_cue(int index, double clip_position_s, std::uint8_t colour = 0);
    void clear_cue(int index);
    const HotCue& cue(int index) const;
    // Jumps playback to a cue. Returns false when that cue is unset.
    bool jump_to_cue(int index);

    // ---- beat jump ----
    void beat_jump(double beats);

    // ---- slip ----
    // While slip is engaged, playback carries on underneath a loop or a jump as
    // if nothing had happened; releasing it lands where the clip would have been.
    void set_slip(bool on);
    bool slip() const { return slip_; }
    double shadow_position_s() const;

    double position_s() const { return position_s_; }
    bool quantise() const { return quantise_; }
    void set_quantise(bool on) { quantise_ = on; }

private:
    void shift(double delta_s);
    double quantised(double seconds) const;

    double clip_duration_s_ = 0.0;
    BeatGrid grid_;

    bool have_platter_ = false;
    double platter_s_ = 0.0;

    // Output = platter + offset. Every feature here moves this one number.
    double offset_s_ = 0.0;
    // What the offset would be had slip-protected actions never happened.
    double shadow_offset_s_ = 0.0;

    double position_s_ = 0.0;
    Loop loop_;
    std::array<HotCue, kHotCueCount> cues_{};
    bool slip_ = false;
    bool quantise_ = true;
};

}  // namespace svj
