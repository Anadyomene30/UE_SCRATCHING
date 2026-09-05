#include "core/playback.h"

#include <cmath>

namespace svj {
namespace {

// fmod that returns into [0, span) rather than keeping the sign of the input.
double wrap(double value, double span) {
    const double rest = std::fmod(value, span);
    return rest < 0.0 ? rest + span : rest;
}

}  // namespace

FoldedPosition fold_position(double timeline_s, double duration_s, ClipPlayMode mode) {
    FoldedPosition out;
    if (duration_s <= 0.0) return out;

    switch (mode) {
        case ClipPlayMode::Once:
            if (timeline_s < 0.0) {
                out.position_s = 0.0;
                out.finished = true;
            } else if (timeline_s >= duration_s) {
                out.position_s = duration_s;
                out.finished = true;
            } else {
                out.position_s = timeline_s;
            }
            return out;

        case ClipPlayMode::PingPong: {
            // One period is out and back, so the fold is a triangle wave. Doing
            // it in closed form rather than by flipping a direction flag at the
            // ends is what lets a ping-pong deck be scrubbed: ask it where it is
            // at any time, in any order, and the answer is the same.
            const double period = 2.0 * duration_s;
            const double u = wrap(timeline_s, period);
            if (u <= duration_s) {
                out.position_s = u;
            } else {
                out.position_s = period - u;
                out.reversed = true;
            }
            return out;
        }

        case ClipPlayMode::Loop:
        default:
            out.position_s = wrap(timeline_s, duration_s);
            return out;
    }
}

// --- DeckClock ---------------------------------------------------------------

void DeckClock::configure(double duration_s, double bpm) {
    duration_s_ = duration_s;
    bpm_ = bpm;
    mode_ = ClipPlayMode::Loop;
    source_ = DeckSource::Timecode;
    takeover_ = TakeoverMode::Grab;
    rate_ = 1.0;
    beats_per_cycle_ = 0.0;
    origin_time_s_ = 0.0;
    origin_position_s_ = 0.0;
    takeover_offset_s_ = 0.0;
    last_position_s_ = 0.0;
}

double DeckClock::effective_rate() const {
    if (source_ != DeckSource::TempoLocked) return rate_;
    if (bpm_ <= 0.0 || beats_per_cycle_ <= 0.0 || duration_s_ <= 0.0) return rate_;

    // The clip is stretched so one pass through it lasts `beats_per_cycle_`
    // beats. Only the SIGN of rate_ survives: under tempo lock the magnitude is
    // the grid's business, and letting both apply would silently double-scale.
    const double cycle_s = beats_per_cycle_ * 60.0 / bpm_;
    const double magnitude = duration_s_ / cycle_s;
    return rate_ < 0.0 ? -magnitude : magnitude;
}

double DeckClock::timeline_at(double time_s) const {
    return origin_position_s_ + effective_rate() * (time_s - origin_time_s_);
}

void DeckClock::seek(double position_s, double time_s) {
    origin_position_s_ = position_s;
    origin_time_s_ = time_s;
}

void DeckClock::set_rate(double rate, double time_s) {
    // Rebase before the change, so the position at `time_s` is identical either
    // side of it. Without this the closed form would tear on every rate move.
    seek(timeline_at(time_s), time_s);
    rate_ = rate;
}

void DeckClock::set_beats_per_cycle(double beats, double time_s) {
    seek(timeline_at(time_s), time_s);
    beats_per_cycle_ = beats;
}

void DeckClock::set_bpm(double bpm, double time_s) {
    seek(timeline_at(time_s), time_s);
    bpm_ = bpm;
}

void DeckClock::set_source(DeckSource source, double time_s) {
    if (source == source_) return;
    // Hold the picture across the switch. Leaving the platter means the free
    // clock starts from where the deck already is; joining it is the takeover
    // policy's business, not this function's.
    if (source != DeckSource::Timecode) seek(last_position_s_, time_s);
    source_ = source;
}

SourceReading DeckClock::read_source(double time_s, double platter_position_s,
                                     float platter_velocity) const {
    SourceReading reading;
    if (source_ == DeckSource::Timecode) {
        reading.position_s = platter_position_s + takeover_offset_s_;
        reading.rate = static_cast<double>(platter_velocity);
    } else {
        reading.position_s = timeline_at(time_s);
        reading.rate = effective_rate();
    }
    return reading;
}

ClockOutput DeckClock::resolve(double played_s, double source_rate) {
    const FoldedPosition folded = fold_position(played_s, duration_s_, mode_);

    ClockOutput out;
    out.position_s = folded.position_s;
    out.reversed = folded.reversed;
    out.finished = folded.finished;
    // A parked clip is not moving, whatever the source still claims. A ping-pong
    // return leg is the clip in reverse, so its velocity is too.
    out.velocity = folded.finished ? 0.0 : (folded.reversed ? -source_rate : source_rate);

    last_position_s_ = out.position_s;
    return out;
}

void DeckClock::hand_over_to_timecode(double platter_position_s, double time_s) {
    (void)time_s;

    switch (takeover_) {
        case TakeoverMode::Ignore:
            // The platter is not this deck's business. Nothing happens until the
            // source is switched by hand, which is what a background layer that
            // must never react to a stray touch wants.
            return;

        case TakeoverMode::Jump:
            takeover_offset_s_ = 0.0;
            break;

        case TakeoverMode::Grab:
        default:
            // played = platter + offset, with the offset chosen so the picture
            // does not move on the frame the hand lands. The price, paid
            // knowingly, is that the platter position stops meaning anything
            // absolute -- exactly the trade core/anchor already makes for the
            // follower mode, and for the same reason: a jump on screen is worse
            // than a number that no longer maps to the label on the record.
            takeover_offset_s_ = last_position_s_ - platter_position_s;
            break;
    }

    source_ = DeckSource::Timecode;
}

}  // namespace svj
