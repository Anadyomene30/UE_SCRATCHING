#include "core/timecode.h"
#include "harness.h"

using namespace svj;

namespace {

DecoderSample locked(double t, double pos, float pitch, float level = 1.0f) {
    return DecoderSample{t, pos, pitch, level};
}

DecoderSample silent(double t) { return DecoderSample{t, -1.0, 0.0f, 0.0f}; }

// Plays forward at nominal speed from `from` for `count` blocks of `dt`.
void play(TimecodeTracker& tracker, double& t, double& pos, int count, double dt = 0.01) {
    for (int i = 0; i < count; ++i) {
        t += dt;
        pos += dt;
        tracker.submit(locked(t, pos, 1.0f));
    }
}

}  // namespace

SVJ_TEST("timecode: absolute mode follows the position on the record") {
    TimecodeConfig config;
    config.mode = TransportMode::Absolute;
    TimecodeTracker tracker(config);

    double t = 0.0, pos = 10.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 10);

    CHECK_NEAR(tracker.state().position_s, 10.10, 1e-6);
    CHECK_NEAR(tracker.state().velocity, 1.0, 1e-6);
    CHECK(tracker.state().link == LinkState::Ok);
}

SVJ_TEST("timecode: a stopped platter on VINYL is normal, not a fault") {
    TimecodeConfig config;
    config.profile = SignalProfile::Vinyl;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 5.0, 1.0f));
    const auto& state = tracker.submit(silent(0.01));

    CHECK(state.link == LinkState::Ok);
    CHECK(state.platter_stopped);
    CHECK_NEAR(state.confidence, 1.0, 1e-6);
    CHECK_NEAR(state.position_s, 5.0, 1e-6);
}

SVJ_TEST("timecode: the same silence on WIRELESS is a lost link, not a stopped platter") {
    // The Phase dock emits continuously, so silence means the radio link is gone.
    // Reading it as a stopped platter would freeze the picture with no warning.
    TimecodeConfig config;
    config.profile = SignalProfile::Wireless;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 5.0, 1.0f));
    const auto& state = tracker.submit(silent(0.01));

    CHECK(state.link == LinkState::Lost);
    CHECK(!state.platter_stopped);
    CHECK_NEAR(state.confidence, 0.0, 1e-6);
}

SVJ_TEST("timecode: a stopped platter on WIRELESS still carries a signal") {
    TimecodeConfig config;
    config.profile = SignalProfile::Wireless;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 5.0, 1.0f));
    const auto& state = tracker.submit(locked(0.01, 5.0, 0.0f));

    CHECK(state.link == LinkState::Ok);
    CHECK(state.platter_stopped);
}

SVJ_TEST("timecode: a lost link freezes rather than decays") {
    // Decaying towards zero would still move the picture, which is the one thing
    // a dropout must never do.
    TimecodeTracker tracker;
    double t = 0.0, pos = 3.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 5);
    const double frozen = tracker.state().position_s;

    for (int i = 0; i < 50; ++i) {
        t += 0.01;
        tracker.submit(silent(t));
    }
    CHECK_NEAR(tracker.state().position_s, frozen, 1e-9);
    CHECK_NEAR(tracker.state().velocity, 0.0, 1e-9);
}

SVJ_TEST("timecode: a carrier without readable bits coasts on pitch") {
    TimecodeTracker tracker;
    tracker.submit(locked(0.0, 2.0, 1.0f));

    // Signal present, position unreadable.
    const auto& state = tracker.submit(DecoderSample{0.10, -1.0, 1.0f, 0.9f});
    CHECK(state.link == LinkState::Degraded);
    CHECK_NEAR(state.position_s, 2.10, 1e-6);
    CHECK(state.confidence > 0.0f);
    CHECK(state.confidence < 1.0f);
}

SVJ_TEST("timecode: relative mode absorbs a lifted and replaced remote") {
    // On a Phase you do not drop a needle, you lift the remote and put it back.
    // Relative mode must swallow that, which is why it matters more here.
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);

    double t = 0.0, pos = 20.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 10);
    const double before = tracker.state().position_s;

    // Remote replaced far away on the virtual record.
    t += 0.01;
    pos = 90.0;
    const auto& state = tracker.submit(locked(t, pos, 1.0f));

    CHECK(state.jumped);
    CHECK_NEAR(state.position_s, before, 1e-6);  // output did not move
    CHECK_EQ(tracker.jump_count(), 1);

    // And playback continues normally from there.
    play(tracker, t, pos, 10);
    CHECK_NEAR(tracker.state().position_s, before + 0.10, 1e-6);
}

SVJ_TEST("timecode: absolute mode glides across a discontinuity instead of snapping") {
    TimecodeConfig config;
    config.mode = TransportMode::Absolute;
    config.relock_ramp_s = 0.10;
    TimecodeTracker tracker(config);

    double t = 0.0, pos = 20.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 5);
    const double before = tracker.state().position_s;

    t += 0.01;
    pos = 90.0;
    tracker.submit(locked(t, pos, 1.0f));
    // Part way there, not all the way, and not still at the old spot.
    CHECK(tracker.state().position_s > before);
    CHECK(tracker.state().position_s < 90.0);

    play(tracker, t, pos, 30);
    CHECK_NEAR(tracker.state().position_s, pos, 1e-3);  // settled on the truth
}

SVJ_TEST("timecode: ordinary playback is not mistaken for a discontinuity") {
    TimecodeTracker tracker;
    double t = 0.0, pos = 0.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 200);
    CHECK_EQ(tracker.jump_count(), 0);
}

SVJ_TEST("timecode: a fast scratch is not mistaken for a discontinuity") {
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);

    double t = 0.0, pos = 30.0;
    tracker.submit(locked(t, pos, 1.0f));
    for (int i = 0; i < 40; ++i) {
        const float pitch = (i % 2 == 0) ? -6.0f : 6.0f;
        t += 0.01;
        pos += pitch * 0.01;
        tracker.submit(locked(t, pos, pitch));
    }
    CHECK_EQ(tracker.jump_count(), 0);
}

SVJ_TEST("timecode: internal mode ignores the platter entirely") {
    TimecodeConfig config;
    config.mode = TransportMode::Internal;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 0.0, 1.0f));
    for (int i = 1; i <= 100; ++i) tracker.submit(silent(i * 0.01));

    CHECK_NEAR(tracker.state().position_s, 1.0, 1e-6);
    CHECK_NEAR(tracker.state().velocity, 1.0, 1e-6);
    CHECK(tracker.state().link == LinkState::Ok);
}

SVJ_TEST("timecode: switching mode does not move the playhead") {
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);

    double t = 0.0, pos = 50.0;
    tracker.submit(locked(t, pos, 1.0f));
    play(tracker, t, pos, 10);
    const double before = tracker.state().position_s;

    tracker.set_mode(TransportMode::Absolute);
    t += 0.01;
    pos += 0.01;
    tracker.submit(locked(t, pos, 1.0f));
    CHECK_NEAR(tracker.state().position_s, before + 0.01, 1e-6);
}

SVJ_TEST("timecode: the link recovers when the carrier comes back") {
    TimecodeTracker tracker;
    double t = 0.0, pos = 8.0;
    tracker.submit(locked(t, pos, 1.0f));

    for (int i = 0; i < 20; ++i) tracker.submit(silent(t += 0.01));
    CHECK(tracker.state().link == LinkState::Lost);

    t += 0.01;
    tracker.submit(locked(t, pos, 1.0f));
    CHECK(tracker.state().link == LinkState::Ok);
    CHECK_NEAR(tracker.state().confidence, 1.0, 1e-6);
}

SVJ_TEST("timecode: reset clears the jump history") {
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);
    tracker.submit(locked(0.0, 1.0, 1.0f));
    tracker.submit(locked(0.01, 40.0, 1.0f));
    CHECK_EQ(tracker.jump_count(), 1);

    tracker.reset();
    CHECK_EQ(tracker.jump_count(), 0);
    CHECK(tracker.state().link == LinkState::Lost);
}

SVJ_TEST("timecode: coming back from a dropout is not a jump") {
    // Dropouts are a fact of life on a wireless link. Counting each recovery as a
    // discontinuity would age the follower-mode anchor for no reason at all.
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 10.0, 1.0f));
    tracker.submit(locked(0.01, 10.01, 1.0f));
    const double before = tracker.state().position_s;

    for (int i = 0; i < 100; ++i) tracker.submit(silent(0.02 + i * 0.01));
    CHECK(tracker.state().link == LinkState::Lost);

    // The remote comes back somewhere else entirely, as it will.
    tracker.submit(locked(1.10, 47.0, 1.0f));
    CHECK(tracker.state().link == LinkState::Ok);
    CHECK_EQ(tracker.jump_count(), 0);
    CHECK_NEAR(tracker.state().position_s, before, 1e-6);

    // And playback carries on smoothly from there.
    tracker.submit(locked(1.11, 47.01, 1.0f));
    CHECK_NEAR(tracker.state().position_s, before + 0.01, 1e-6);
    CHECK_EQ(tracker.jump_count(), 0);
}

SVJ_TEST("timecode: a real discontinuity is still caught after a clean recovery") {
    TimecodeConfig config;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);

    tracker.submit(locked(0.0, 10.0, 1.0f));
    for (int i = 0; i < 10; ++i) tracker.submit(silent(0.01 + i * 0.01));
    tracker.submit(locked(0.12, 40.0, 1.0f));   // recovery, not a jump
    CHECK_EQ(tracker.jump_count(), 0);

    tracker.submit(locked(0.13, 90.0, 1.0f));   // this one really is a jump
    CHECK_EQ(tracker.jump_count(), 1);
}
