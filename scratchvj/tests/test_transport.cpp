#include "core/transport.h"
#include "harness.h"

using namespace svj;

namespace {

BeatGrid grid_120() { return BeatGrid{120.0, 0.0}; }  // 0.5 s per beat

Transport make(double duration = 300.0, BeatGrid grid = grid_120()) {
    Transport t;
    t.configure(duration, grid);
    t.set_quantise(false);  // most tests want exact positions
    return t;
}

}  // namespace

SVJ_TEST("transport: with nothing engaged the platter plays straight through") {
    Transport t = make();
    CHECK_NEAR(t.map(10.0), 10.0, 1e-9);
    CHECK_NEAR(t.map(11.5), 11.5, 1e-9);
}

SVJ_TEST("transport: a beat grid converts beats to seconds") {
    BeatGrid grid{120.0, 0.25};
    CHECK(grid.valid());
    CHECK_NEAR(grid.beat_duration_s(), 0.5, 1e-9);
    CHECK_NEAR(grid.snap(0.30), 0.25, 1e-9);
    CHECK_NEAR(grid.snap(0.60), 0.75, 1e-9);
    CHECK_NEAR(grid.snap(1.20), 1.25, 1e-9);
}

SVJ_TEST("transport: an invalid grid leaves times alone rather than dividing by zero") {
    BeatGrid none;
    CHECK(!none.valid());
    CHECK_NEAR(none.snap(3.7), 3.7, 1e-9);
    CHECK_NEAR(none.beat_duration_s(), 0.0, 1e-9);
}

SVJ_TEST("transport: a hot cue jumps playback and keeps playing from there") {
    Transport t = make();
    t.map(50.0);
    t.set_cue(0, 12.0);
    CHECK(t.cue(0).set);

    CHECK(t.jump_to_cue(0));
    CHECK_NEAR(t.position_s(), 12.0, 1e-9);

    // The platter carries on from where it was; playback follows relative to it.
    CHECK_NEAR(t.map(51.0), 13.0, 1e-9);
}

SVJ_TEST("transport: jumping to an unset cue does nothing") {
    Transport t = make();
    t.map(20.0);
    CHECK(!t.jump_to_cue(3));
    CHECK_NEAR(t.map(21.0), 21.0, 1e-9);
}

SVJ_TEST("transport: cue indices outside the bank are ignored safely") {
    Transport t = make();
    t.set_cue(-1, 5.0);
    t.set_cue(99, 5.0);
    CHECK(!t.jump_to_cue(-1));
    CHECK(!t.jump_to_cue(99));
    CHECK(!t.cue(42).set);
}

SVJ_TEST("transport: clearing a cue unsets it") {
    Transport t = make();
    t.set_cue(2, 9.0);
    t.clear_cue(2);
    CHECK(!t.cue(2).set);
    CHECK(!t.jump_to_cue(2));
}

SVJ_TEST("transport: a beat jump moves by whole beats in either direction") {
    Transport t = make();
    t.map(10.0);
    t.beat_jump(4);  // 4 beats at 120 bpm = 2 s
    CHECK_NEAR(t.position_s(), 12.0, 1e-9);
    CHECK_NEAR(t.map(11.0), 13.0, 1e-9);

    t.beat_jump(-8);
    CHECK_NEAR(t.map(11.0), 9.0, 1e-9);
}

SVJ_TEST("transport: a loop folds playback back on itself") {
    Transport t = make();
    t.map(10.0);
    t.loop_in(10.0);
    t.loop_out(12.0);
    CHECK(t.loop().active);
    CHECK_NEAR(t.loop().length_s(), 2.0, 1e-9);

    CHECK_NEAR(t.map(11.0), 11.0, 1e-9);
    CHECK_NEAR(t.map(12.5), 10.5, 1e-9);  // wrapped
    CHECK_NEAR(t.map(13.0), 11.0, 1e-9);
}

SVJ_TEST("transport: a scratch crossing a short loop many times still lands right") {
    // A single wrap would not be enough: a fast hand can cross a short loop
    // several times within one audio block.
    Transport t = make();
    t.map(10.0);
    t.loop_in(10.0);
    t.loop_out(10.25);

    const double played = t.map(10.0 + 7.6);  // 30.4 loop lengths later
    CHECK(played >= 10.0);
    CHECK(played < 10.25);
    CHECK_NEAR(played, 10.1, 1e-6);
}

SVJ_TEST("transport: a loop wraps going backwards too") {
    Transport t = make();
    t.map(10.0);
    t.loop_in(10.0);
    t.loop_out(12.0);
    const double played = t.map(9.5);
    CHECK(played >= 10.0);
    CHECK(played < 12.0);
    CHECK_NEAR(played, 11.5, 1e-9);
}

SVJ_TEST("transport: a backwards loop is refused rather than trapping playback") {
    Transport t = make();
    t.map(10.0);
    t.loop_in(12.0);
    t.loop_out(11.0);
    CHECK(!t.loop().active);
    CHECK_NEAR(t.map(13.0), 13.0, 1e-9);
}

SVJ_TEST("transport: an auto loop is exactly the requested number of beats") {
    Transport t = make();
    t.map(20.0);
    t.auto_loop(4);  // 2 s at 120 bpm
    CHECK(t.loop().active);
    CHECK_NEAR(t.loop().start_s, 20.0, 1e-9);
    CHECK_NEAR(t.loop().length_s(), 2.0, 1e-9);
}

SVJ_TEST("transport: an auto loop needs a beat grid") {
    Transport t = make(300.0, BeatGrid{});
    t.map(20.0);
    t.auto_loop(4);
    CHECK(!t.loop().active);
}

SVJ_TEST("transport: leaving a loop WITHOUT slip carries on from the loop") {
    Transport t = make();
    t.map(10.0);
    t.loop_in(10.0);
    t.loop_out(11.0);

    t.map(13.5);  // three and a half loops later, playing 10.5
    CHECK_NEAR(t.position_s(), 10.5, 1e-9);

    t.exit_loop();
    CHECK_NEAR(t.map(14.0), 11.0, 1e-9);  // continues from where the loop was
}

SVJ_TEST("transport: leaving a loop WITH slip lands where the clip would have been") {
    // The whole point of slip: the clip never stopped underneath.
    Transport t = make();
    t.map(10.0);
    t.set_slip(true);
    t.loop_in(10.0);
    t.loop_out(11.0);

    t.map(13.5);
    CHECK_NEAR(t.position_s(), 10.5, 1e-9);        // heard inside the loop
    CHECK_NEAR(t.shadow_position_s(), 13.5, 1e-9); // where it would have been

    t.exit_loop();
    CHECK_NEAR(t.map(13.5), 13.5, 1e-9);
}

SVJ_TEST("transport: slip also covers a hot cue jump") {
    Transport t = make();
    t.map(30.0);
    t.set_slip(true);
    t.set_cue(0, 5.0);
    t.jump_to_cue(0);
    CHECK_NEAR(t.position_s(), 5.0, 1e-9);

    t.map(31.0);
    CHECK_NEAR(t.position_s(), 6.0, 1e-9);
    t.set_slip(false);
    CHECK_NEAR(t.position_s(), 31.0, 1e-9);
}

SVJ_TEST("transport: releasing slip when nothing happened changes nothing") {
    Transport t = make();
    t.map(40.0);
    t.set_slip(true);
    t.map(41.0);
    t.set_slip(false);
    CHECK_NEAR(t.map(42.0), 42.0, 1e-9);
}

SVJ_TEST("transport: quantise snaps a loop to the grid, and can be turned off") {
    Transport on;
    on.configure(300.0, BeatGrid{120.0, 0.0});
    on.map(10.13);
    on.loop_in(10.13);
    on.loop_out(12.09);
    CHECK_NEAR(on.loop().start_s, 10.0, 1e-9);
    CHECK_NEAR(on.loop().end_s, 12.0, 1e-9);

    Transport off = make();
    off.map(10.13);
    off.loop_in(10.13);
    off.loop_out(12.09);
    CHECK_NEAR(off.loop().start_s, 10.13, 1e-9);
}

SVJ_TEST("transport: a cue set with quantise on lands on a beat") {
    Transport t;
    t.configure(300.0, BeatGrid{120.0, 0.0});
    t.set_cue(1, 7.31);
    CHECK_NEAR(t.cue(1).position_s, 7.5, 1e-9);
}

SVJ_TEST("transport: reset clears loops, cues and slip") {
    Transport t = make();
    t.map(10.0);
    t.set_cue(0, 1.0);
    t.loop_in(10.0);
    t.loop_out(11.0);
    t.set_slip(true);

    t.reset();
    CHECK(!t.loop().active);
    CHECK(!t.cue(0).set);
    CHECK(!t.slip());
    CHECK_NEAR(t.map(55.0), 55.0, 1e-9);
}

SVJ_TEST("transport: a loop survives scratching inside it and stays in bounds") {
    Transport t = make();
    t.map(10.0);
    t.loop_in(10.0);
    t.loop_out(10.5);

    double platter = 10.0;
    for (int i = 0; i < 300; ++i) {
        platter += (i % 2 == 0) ? -0.13 : 0.21;
        const double played = t.map(platter);
        CHECK(played >= 10.0 - 1e-9);
        CHECK(played < 10.5 + 1e-9);
    }
}
