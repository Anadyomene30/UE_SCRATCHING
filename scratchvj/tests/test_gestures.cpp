#include "core/gestures.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("gestures: steady nominal playback is not scratching") {
    GestureTracker tracker;
    for (int i = 0; i < 50; ++i) tracker.update(i * 0.01, 1.0f, 1.0f);
    CHECK(!tracker.scratching());
    CHECK(!tracker.backspin());
    CHECK_NEAR(tracker.scratch_rate(), 0.0, 1e-6);
}

SVJ_TEST("gestures: a platter worked away from nominal counts as scratching") {
    GestureTracker tracker;
    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.01, 0.2f, 1.0f);
    CHECK(tracker.scratching());
}

SVJ_TEST("gestures: direction reversals are counted per second") {
    GestureConfig config;
    config.scratch_window_s = 1.0f;
    GestureTracker tracker(config);

    // Six reversals inside a one second window.
    double t = 0.0;
    tracker.update(t, 1.0f, 1.0f);
    for (int i = 0; i < 6; ++i) {
        t += 0.05;
        tracker.update(t, (i % 2 == 0) ? -1.0f : 1.0f, 1.0f);
    }
    CHECK_NEAR(tracker.scratch_rate(), 6.0, 1e-5);
}

SVJ_TEST("gestures: reversals older than the window are forgotten") {
    GestureConfig config;
    config.scratch_window_s = 0.5f;
    GestureTracker tracker(config);

    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.05, -1.0f, 1.0f);
    tracker.update(0.10, 1.0f, 1.0f);
    CHECK_NEAR(tracker.scratch_rate(), 4.0, 1e-5);  // 2 reversals / 0.5 s

    // Well past the window with no further reversals.
    tracker.update(2.0, 1.0f, 1.0f);
    CHECK_NEAR(tracker.scratch_rate(), 0.0, 1e-6);
}

SVJ_TEST("gestures: a backspin needs both the speed and the duration") {
    GestureConfig config;
    config.backspin_velocity = -2.5f;
    config.backspin_hold_s = 0.2f;
    GestureTracker tracker(config);

    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.05, -4.0f, 1.0f);
    CHECK(!tracker.backspin());  // fast enough, but not yet long enough

    tracker.update(0.30, -4.0f, 1.0f);
    CHECK(tracker.backspin());
}

SVJ_TEST("gestures: leaving backspin speed clears the flag and restarts the timer") {
    GestureTracker tracker;
    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.1, -4.0f, 1.0f);
    tracker.update(0.5, -4.0f, 1.0f);
    CHECK(tracker.backspin());

    tracker.update(0.6, -1.0f, 1.0f);
    CHECK(!tracker.backspin());

    tracker.update(0.65, -4.0f, 1.0f);
    CHECK(!tracker.backspin());  // the hold time starts again from here
}

SVJ_TEST("gestures: losing timecode confidence freezes the output instead of moving it") {
    // A Phase dropout must never teleport the picture or the camera.
    GestureTracker tracker;
    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.01, 0.5f, 1.0f);
    const float held_velocity = tracker.velocity();
    CHECK(!tracker.holding());

    tracker.update(0.02, -9.0f, 0.0f);  // garbage arriving with no lock
    CHECK(tracker.holding());
    CHECK_NEAR(tracker.velocity(), held_velocity, 1e-6);
}

SVJ_TEST("gestures: confidence returning resumes tracking") {
    GestureTracker tracker;
    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.01, -9.0f, 0.0f);
    CHECK(tracker.holding());

    tracker.update(0.02, 0.5f, 1.0f);
    CHECK(!tracker.holding());
    CHECK_NEAR(tracker.velocity(), 0.5, 1e-6);
}

SVJ_TEST("gestures: acceleration follows the sign of the change in velocity") {
    GestureConfig config;
    config.accel_smoothing_ms = 0.0f;  // measure the raw derivative
    GestureTracker tracker(config);

    tracker.update(0.0, 0.0f, 1.0f);
    tracker.update(0.1, 1.0f, 1.0f);
    CHECK_NEAR(tracker.acceleration(), 10.0, 1e-4);

    tracker.update(0.2, 0.0f, 1.0f);
    CHECK_NEAR(tracker.acceleration(), -10.0, 1e-4);
}

SVJ_TEST("gestures: a repeated timestamp does not divide by zero") {
    GestureTracker tracker;
    tracker.update(1.0, 0.5f, 1.0f);
    tracker.update(1.0, 0.9f, 1.0f);
    CHECK_NEAR(tracker.velocity(), 0.9, 1e-6);
    CHECK(std::isfinite(tracker.acceleration()));
}

SVJ_TEST("gestures: a platter resting at zero does not chatter reversals") {
    GestureTracker tracker;
    for (int i = 0; i < 40; ++i) tracker.update(i * 0.01, 0.0f, 1.0f);
    CHECK_NEAR(tracker.scratch_rate(), 0.0, 1e-6);
}

SVJ_TEST("gestures: reset clears every derived value") {
    GestureTracker tracker;
    tracker.update(0.0, 1.0f, 1.0f);
    tracker.update(0.05, -3.0f, 1.0f);
    tracker.reset();
    CHECK_NEAR(tracker.velocity(), 0.0, 1e-6);
    CHECK_NEAR(tracker.scratch_rate(), 0.0, 1e-6);
    CHECK(!tracker.backspin());
    CHECK(!tracker.holding());
}
