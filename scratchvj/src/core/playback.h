// scratchvj — where a deck's position comes from, and what the clip does at its
// ends.
//
// Until now a deck was a platter and nothing else: Deck::advance() took a
// timecode sample, so every deck followed a turntable by construction. That is
// wrong for the layer this instrument is missing -- a background texture, a
// logo, a grain loop -- which has to run on its own while the two scratch decks
// are being worked. So the position source becomes a per-deck choice.
//
// The trap is that a free-running deck is, in the obvious implementation, an
// INTEGRATOR: pos += rate * dt. That is precisely what this project forbids,
// because an integrator cannot be scratched, cannot be replayed identically by
// core/take, and drifts over a set. So free-run here is a closed form of
// absolute time:
//
//     timeline = origin_position + rate * (now - origin_time)
//
// Changing the rate rebases the origin so the position stays continuous, and
// nothing is ever accumulated. Three hours in, the position is still exact.
//
// The play mode is then not a player but a FOLD on position -- one pure
// function, applied identically to a free-running background and to a scratched
// deck whose platter has run past the end of the clip.
#pragma once

#include <cstdint>

namespace svj {

// What the clip does when the timeline leaves it.
enum class ClipPlayMode : std::uint8_t {
    Loop,      // wraps, however many times over
    PingPong,  // plays back and forth; the return leg is the clip in reverse
    Once,      // holds at the boundary it ran off
};

struct FoldedPosition {
    double position_s = 0.0;

    // Ping-pong plays the clip BACKWARDS on the return leg. Anything downstream
    // that cares about direction -- the gesture tracker, a chroma flip on
    // reversal, the frame window's prefetch direction -- would otherwise read
    // the timeline's sign and be wrong half the time.
    bool reversed = false;

    // Once mode has run off an end and is parked there.
    bool finished = false;
};

// Folds an arbitrary timeline position into [0, duration]. Handles a timeline
// several clip lengths outside the clip in either direction, because a scratch
// on a two-second loop crosses it repeatedly inside one block.
FoldedPosition fold_position(double timeline_s, double duration_s, ClipPlayMode mode);

// Where a deck's position comes from.
enum class DeckSource : std::uint8_t {
    Timecode,    // the platter, through the timecode tracker -- the scratch decks
    FreeRun,     // its own clock at its own rate, ignoring the platter entirely
    TempoLocked, // its own clock, but with the clip stretched to a beat count
};

// What a deck does when the platter comes alive under a clip that was running
// free. See DeckClock::hand_over_to_timecode().
enum class TakeoverMode : std::uint8_t {
    Ignore,  // the platter is dead to this deck until the source is switched by hand
    Jump,    // the clip snaps to wherever the platter says it is
    Grab,    // the platter takes over from where the clip already is
};

// What the source is asking for, before the transport has had its say.
struct SourceReading {
    // Unfolded: it may sit outside the clip, by several clip lengths.
    double position_s = 0.0;
    // Clip seconds per wall second, signed, before the fold.
    double rate = 0.0;
};

struct ClockOutput {
    double position_s = 0.0;
    // Signed clip seconds per wall second, after the fold. Zero once a Once clip
    // is parked. This is what the frame window and anything direction-sensitive
    // want; they must not re-derive it from successive positions, which would
    // read a loop wrap as a backspin.
    double velocity = 0.0;
    bool reversed = false;
    bool finished = false;
};

// A deck's clock. Owns the source, the play mode and the free-run rate; knows
// nothing about timecode decoding, frames or the mixer.
//
// It is deliberately NOT one advance() call. The clip-boundary fold has to
// happen AFTER core/transport has applied loops, hot cues and slip -- a user
// loop straddling the end of the clip would otherwise be folded away before the
// transport ever saw it. So the clock reads the source, the transport runs, and
// the clock resolves the result. The seam is the point.
class DeckClock {
public:
    // `bpm` is only consulted by TempoLocked, and may be zero otherwise. Resets
    // the mode, the source and the rate along with the clip.
    void configure(double duration_s, double bpm);

    void set_mode(ClipPlayMode mode) { mode_ = mode; }
    ClipPlayMode mode() const { return mode_; }

    void set_takeover(TakeoverMode takeover) { takeover_ = takeover; }
    TakeoverMode takeover() const { return takeover_; }

    DeckSource source() const { return source_; }
    // Switches source at `time_s`, holding the position the deck is already at
    // so that the switch itself never moves the picture.
    void set_source(DeckSource source, double time_s);

    // Free-run rate, 1.0 nominal, negative to run backwards. Rebases the origin
    // so the clip does not jump when the rate changes under it.
    void set_rate(double rate, double time_s);
    double rate() const { return rate_; }

    // How many beats one pass through the clip should take when TempoLocked. A
    // twelve-second grain loop set to eight beats at 120 bpm plays at 3x.
    void set_beats_per_cycle(double beats, double time_s);
    double beats_per_cycle() const { return beats_per_cycle_; }
    void set_bpm(double bpm, double time_s);

    // Clip seconds per wall second the free-running clock runs at. Under
    // Timecode nothing consults it: the platter sets the rate.
    double effective_rate() const;

    // Puts the clip at `position_s` now, without touching the rate. Used on
    // load, on a cue jump, and by a takeover.
    void seek(double position_s, double time_s);

    // The unfolded free-run timeline at `time_s`. Public because it is the whole
    // claim of this file -- a function of absolute time, never an accumulator --
    // and a test says so directly.
    double timeline_at(double time_s) const;

    // Step one: what the source asks for. The platter arguments are ignored
    // unless the source is Timecode.
    SourceReading read_source(double time_s, double platter_position_s,
                              float platter_velocity) const;

    // Step two: the transport's output, folded into the clip. `source_rate` is
    // the rate that came out of read_source().
    ClockOutput resolve(double played_s, double source_rate);

    // Called when the platter locks under a deck that is not following it. What
    // this does is the TakeoverMode policy.
    void hand_over_to_timecode(double platter_position_s, double time_s);

    // The offset a Grab established: played = platter + offset. Zero otherwise.
    double takeover_offset_s() const { return takeover_offset_s_; }

private:
    double duration_s_ = 0.0;
    double bpm_ = 0.0;
    ClipPlayMode mode_ = ClipPlayMode::Loop;
    DeckSource source_ = DeckSource::Timecode;
    TakeoverMode takeover_ = TakeoverMode::Grab;

    double rate_ = 1.0;
    double beats_per_cycle_ = 0.0;

    // The closed form's two constants. Every rate change rebases them; nothing
    // is ever accumulated into them.
    double origin_time_s_ = 0.0;
    double origin_position_s_ = 0.0;

    double takeover_offset_s_ = 0.0;

    // Where the deck actually was on the last resolve(), so that switching
    // source can hold it.
    double last_position_s_ = 0.0;
};

}  // namespace svj
