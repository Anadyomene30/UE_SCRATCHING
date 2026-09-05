#include <cmath>

#include "core/playback.h"
#include "harness.h"

using namespace svj;

namespace {

// A clock already running free, which is the interesting case for most of these.
DeckClock free_clock(double duration_s, double rate, double bpm = 120.0) {
    DeckClock clock;
    clock.configure(duration_s, bpm);
    clock.set_source(DeckSource::FreeRun, 0.0);
    clock.set_rate(rate, 0.0);
    clock.seek(0.0, 0.0);
    return clock;
}

// One frame through the whole seam: source, no transport, fold.
ClockOutput run(DeckClock& clock, double time_s) {
    const SourceReading reading = clock.read_source(time_s, 0.0, 0.0f);
    return clock.resolve(reading.position_s, reading.rate);
}

}  // namespace

// --- the fold ----------------------------------------------------------------

SVJ_TEST("fold: a loop wraps however far outside the clip it lands") {
    // A scratch on a two-second clip crosses it several times inside one block,
    // so a single wrap would not be enough -- the bug this asserts against.
    CHECK_NEAR(fold_position(2.5, 2.0, ClipPlayMode::Loop).position_s, 0.5, 1e-9);
    CHECK_NEAR(fold_position(20.5, 2.0, ClipPlayMode::Loop).position_s, 0.5, 1e-9);
    CHECK_NEAR(fold_position(-0.5, 2.0, ClipPlayMode::Loop).position_s, 1.5, 1e-9);
    CHECK_NEAR(fold_position(-20.5, 2.0, ClipPlayMode::Loop).position_s, 1.5, 1e-9);
}

SVJ_TEST("fold: ping-pong is a triangle, and the return leg says it is reversed") {
    const double d = 4.0;
    CHECK_NEAR(fold_position(1.0, d, ClipPlayMode::PingPong).position_s, 1.0, 1e-9);
    CHECK(!fold_position(1.0, d, ClipPlayMode::PingPong).reversed);

    // Past the end it comes back down, and says so.
    const FoldedPosition back = fold_position(5.0, d, ClipPlayMode::PingPong);
    CHECK_NEAR(back.position_s, 3.0, 1e-9);
    CHECK(back.reversed);

    // A full out-and-back is one period.
    CHECK_NEAR(fold_position(8.0, d, ClipPlayMode::PingPong).position_s, 0.0, 1e-9);
}

SVJ_TEST("fold: ping-pong many periods out matches one period out") {
    // The property that makes a bouncing deck scrubbable rather than played: it
    // is a function of the timeline, so asking out of order gives one answer.
    const double d = 3.0;
    for (double t = -12.0; t <= 12.0; t += 0.25) {
        const FoldedPosition near_zero = fold_position(t, d, ClipPlayMode::PingPong);
        const FoldedPosition far_away = fold_position(t + 6.0 * d, d, ClipPlayMode::PingPong);
        CHECK_NEAR(near_zero.position_s, far_away.position_s, 1e-9);
        CHECK_EQ(near_zero.reversed, far_away.reversed);
    }
}

SVJ_TEST("fold: once holds at whichever boundary it ran off") {
    CHECK_NEAR(fold_position(9.0, 4.0, ClipPlayMode::Once).position_s, 4.0, 1e-9);
    CHECK(fold_position(9.0, 4.0, ClipPlayMode::Once).finished);

    // Backwards it parks at the head, not at the tail.
    CHECK_NEAR(fold_position(-2.0, 4.0, ClipPlayMode::Once).position_s, 0.0, 1e-9);
    CHECK(fold_position(-2.0, 4.0, ClipPlayMode::Once).finished);

    CHECK(!fold_position(2.0, 4.0, ClipPlayMode::Once).finished);
}

SVJ_TEST("fold: a clip with no duration folds to zero instead of dividing by it") {
    for (const ClipPlayMode mode :
         {ClipPlayMode::Loop, ClipPlayMode::PingPong, ClipPlayMode::Once}) {
        const FoldedPosition out = fold_position(7.0, 0.0, mode);
        CHECK_NEAR(out.position_s, 0.0, 1e-9);
    }
}

// --- the closed form ---------------------------------------------------------

SVJ_TEST("clock: FREE RUN IS A FUNCTION OF TIME, NOT AN ACCUMULATION") {
    // The claim the whole module rests on. Stepping the clock sixty times a
    // second for ten seconds must land exactly where jumping straight to ten
    // seconds lands. An integrator would not: it would carry ten seconds' worth
    // of rounding, and would answer differently depending on how it got there.
    DeckClock stepped = free_clock(1000.0, 1.0);
    for (int i = 1; i <= 600; ++i) run(stepped, i / 60.0);
    const double by_steps = stepped.timeline_at(10.0);

    DeckClock jumped = free_clock(1000.0, 1.0);
    const double by_jump = jumped.timeline_at(10.0);

    CHECK_NEAR(by_steps, by_jump, 1e-12);
    CHECK_NEAR(by_jump, 10.0, 1e-12);
}

SVJ_TEST("clock: three hours in, a free-running deck has not drifted") {
    DeckClock clock = free_clock(600.0, 1.0);
    CHECK_NEAR(clock.timeline_at(10800.0), 10800.0, 1e-9);
    // And the fold of it is still exact, which is what the frame index sees.
    const ClockOutput out = run(clock, 10800.0);
    CHECK_NEAR(out.position_s, std::fmod(10800.0, 600.0), 1e-9);
}

SVJ_TEST("clock: changing the rate does not move the picture") {
    DeckClock clock = free_clock(100.0, 1.0);
    const double before = clock.timeline_at(4.0);
    clock.set_rate(0.25, 4.0);
    CHECK_NEAR(clock.timeline_at(4.0), before, 1e-12);
    // And from there it runs at the new rate.
    CHECK_NEAR(clock.timeline_at(8.0), before + 1.0, 1e-9);
}

SVJ_TEST("clock: a negative rate runs the clip backwards") {
    DeckClock clock = free_clock(10.0, -1.0);
    clock.seek(8.0, 0.0);
    CHECK_NEAR(run(clock, 3.0).position_s, 5.0, 1e-9);
    CHECK(run(clock, 3.0).velocity < 0.0);
}

// --- tempo lock --------------------------------------------------------------

SVJ_TEST("clock: tempo lock stretches the clip to its beat count") {
    // Twelve seconds of grain loop, told to last eight beats at 120 bpm, i.e.
    // four seconds: it has to play at 3x.
    DeckClock clock;
    clock.configure(12.0, 120.0);
    clock.set_source(DeckSource::TempoLocked, 0.0);
    clock.set_beats_per_cycle(8.0, 0.0);
    clock.seek(0.0, 0.0);

    CHECK_NEAR(clock.effective_rate(), 3.0, 1e-9);
    // One cycle later it is back at the top of the clip.
    CHECK_NEAR(run(clock, 4.0).position_s, 0.0, 1e-9);
}

SVJ_TEST("clock: tempo lock keeps the sign of the rate but not its magnitude") {
    // Otherwise a deck left at 2x from free-run would silently play the grid at
    // double speed the moment it was locked, which is the kind of fault that is
    // only ever found on stage.
    DeckClock clock;
    clock.configure(12.0, 120.0);
    clock.set_source(DeckSource::TempoLocked, 0.0);
    clock.set_beats_per_cycle(8.0, 0.0);

    clock.set_rate(2.0, 0.0);
    CHECK_NEAR(clock.effective_rate(), 3.0, 1e-9);
    clock.set_rate(-1.0, 0.0);
    CHECK_NEAR(clock.effective_rate(), -3.0, 1e-9);
}

SVJ_TEST("clock: tempo lock with no grid falls back to the plain rate") {
    DeckClock clock;
    clock.configure(12.0, 0.0);
    clock.set_source(DeckSource::TempoLocked, 0.0);
    clock.set_beats_per_cycle(8.0, 0.0);
    clock.set_rate(1.5, 0.0);
    CHECK_NEAR(clock.effective_rate(), 1.5, 1e-9);
}

// --- what the rest of the engine reads ---------------------------------------

SVJ_TEST("clock: a ping-pong return leg reports a negative velocity") {
    // The frame window prefetches in the direction of travel and effects flip on
    // reversal. Both read this number; on the way back the clip really is being
    // played backwards even though the timeline is still going forwards.
    DeckClock clock = free_clock(4.0, 1.0);
    clock.set_mode(ClipPlayMode::PingPong);

    const ClockOutput out_leg = run(clock, 1.0);
    CHECK(!out_leg.reversed);
    CHECK(out_leg.velocity > 0.0);

    const ClockOutput back_leg = run(clock, 5.0);
    CHECK(back_leg.reversed);
    CHECK_NEAR(back_leg.velocity, -1.0, 1e-9);
}

SVJ_TEST("clock: a parked once-clip reports no velocity at all") {
    DeckClock clock = free_clock(4.0, 1.0);
    clock.set_mode(ClipPlayMode::Once);
    const ClockOutput out = run(clock, 9.0);
    CHECK(out.finished);
    CHECK_NEAR(out.position_s, 4.0, 1e-9);
    CHECK_NEAR(out.velocity, 0.0, 1e-12);
}

SVJ_TEST("clock: a timecode deck ignores its own rate entirely") {
    DeckClock clock;
    clock.configure(100.0, 120.0);
    clock.set_rate(4.0, 0.0);  // set, but the platter is in charge

    const SourceReading reading = clock.read_source(10.0, 33.0, -0.5f);
    CHECK_NEAR(reading.position_s, 33.0, 1e-9);
    CHECK_NEAR(reading.rate, -0.5, 1e-9);
}

// --- taking the platter back ------------------------------------------------

SVJ_TEST("takeover: GRAB DOES NOT MOVE THE PICTURE ON THE FRAME THE HAND LANDS") {
    // The whole point of Grab, and the reason it is the default: you can catch a
    // loop in flight and start scratching it. If the frame of the takeover moved
    // the picture at all, the gesture would read as a glitch instead.
    DeckClock clock = free_clock(60.0, 1.0);
    const ClockOutput before = run(clock, 7.0);
    CHECK_NEAR(before.position_s, 7.0, 1e-9);

    // The platter happens to be nowhere near: it says 31 seconds.
    clock.set_takeover(TakeoverMode::Grab);
    clock.hand_over_to_timecode(31.0, 7.0);
    CHECK(clock.source() == DeckSource::Timecode);

    const SourceReading reading = clock.read_source(7.0, 31.0, 1.0f);
    const ClockOutput after = clock.resolve(reading.position_s, reading.rate);
    CHECK_NEAR(after.position_s, before.position_s, 1e-9);
}

SVJ_TEST("takeover: after a grab the platter moves the clip one for one") {
    DeckClock clock = free_clock(60.0, 1.0);
    run(clock, 7.0);
    clock.set_takeover(TakeoverMode::Grab);
    clock.hand_over_to_timecode(31.0, 7.0);

    // Two seconds of platter is two seconds of clip, from wherever it was held.
    const SourceReading reading = clock.read_source(9.0, 33.0, 1.0f);
    CHECK_NEAR(clock.resolve(reading.position_s, reading.rate).position_s, 9.0, 1e-9);
}

SVJ_TEST("takeover: jump snaps to the platter, offset and all") {
    DeckClock clock = free_clock(60.0, 1.0);
    run(clock, 7.0);
    clock.set_takeover(TakeoverMode::Jump);
    clock.hand_over_to_timecode(31.0, 7.0);

    CHECK_NEAR(clock.takeover_offset_s(), 0.0, 1e-12);
    const SourceReading reading = clock.read_source(7.0, 31.0, 1.0f);
    CHECK_NEAR(clock.resolve(reading.position_s, reading.rate).position_s, 31.0, 1e-9);
}

SVJ_TEST("takeover: ignore leaves the deck running free") {
    // A stray hand on the platter must not capture a background layer.
    DeckClock clock = free_clock(60.0, 1.0);
    run(clock, 7.0);
    clock.set_takeover(TakeoverMode::Ignore);
    clock.hand_over_to_timecode(31.0, 7.0);

    CHECK(clock.source() == DeckSource::FreeRun);
    CHECK_NEAR(run(clock, 9.0).position_s, 9.0, 1e-9);
}

SVJ_TEST("clock: switching to a free source holds the position the deck was at") {
    // Arming a background layer must not make the picture jump; the free clock
    // starts from wherever the deck already is.
    DeckClock clock;
    clock.configure(100.0, 120.0);
    const SourceReading reading = clock.read_source(5.0, 42.0, 1.0f);
    clock.resolve(reading.position_s, reading.rate);

    clock.set_source(DeckSource::FreeRun, 5.0);
    CHECK_NEAR(clock.timeline_at(5.0), 42.0, 1e-9);
    CHECK_NEAR(run(clock, 6.0).position_s, 43.0, 1e-9);
}
