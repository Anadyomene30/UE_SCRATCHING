#include "core/anchor.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("anchor: an unarmed anchor asks to be placed") {
    Anchor anchor;
    CHECK(!anchor.armed());
    CHECK(anchor.needs_reanchor(0.0, 0));
    CHECK_NEAR(anchor.staleness(0.0, 0), 1.0, 1e-6);
}

SVJ_TEST("anchor: placing it maps the record onto the clip") {
    Anchor anchor;
    // The drop lands when the record is at 90 s and the clip should be at 12 s.
    anchor.set(90.0, 12.0, 0.0, 0);
    CHECK(anchor.armed());
    CHECK_NEAR(anchor.offset_s(), 78.0, 1e-9);
    CHECK_NEAR(anchor.clip_position(90.0), 12.0, 1e-9);
    CHECK_NEAR(anchor.clip_position(120.0), 42.0, 1e-9);
    CHECK_NEAR(anchor.clip_position(80.0), 2.0, 1e-9);
}

SVJ_TEST("anchor: a fresh anchor is not stale") {
    Anchor anchor;
    anchor.set(10.0, 0.0, 100.0, 3);
    CHECK_NEAR(anchor.staleness(100.0, 3), 0.0, 1e-6);
    CHECK(!anchor.needs_reanchor(100.0, 3));
}

SVJ_TEST("anchor: staleness grows with age") {
    AnchorConfig config;
    config.stale_after_s = 200.0;
    config.stale_after_jumps = 4;
    Anchor anchor(config);
    anchor.set(0.0, 0.0, 0.0, 0);

    CHECK_NEAR(anchor.staleness(50.0, 0), 0.25, 1e-6);
    CHECK_NEAR(anchor.staleness(100.0, 0), 0.5, 1e-6);
    CHECK_NEAR(anchor.staleness(400.0, 0), 1.0, 1e-6);  // clamped
}

SVJ_TEST("anchor: one discontinuity is enough to break it by default") {
    // Lifting the remote almost certainly broke the correspondence.
    Anchor anchor;
    anchor.set(0.0, 0.0, 0.0, 5);
    CHECK(!anchor.needs_reanchor(1.0, 5));
    CHECK_NEAR(anchor.staleness(1.0, 6), 1.0, 1e-6);
    CHECK(anchor.needs_reanchor(1.0, 6));
}

SVJ_TEST("anchor: age and jumps are combined by the worse of the two") {
    // Not summed: either cause alone is enough, and summing would let two mild
    // causes raise a false alarm.
    AnchorConfig config;
    config.stale_after_s = 100.0;
    config.stale_after_jumps = 10;
    Anchor anchor(config);
    anchor.set(0.0, 0.0, 0.0, 0);

    CHECK_NEAR(anchor.staleness(40.0, 3), 0.4, 1e-6);  // 0.4 vs 0.3
    CHECK_NEAR(anchor.staleness(20.0, 7), 0.7, 1e-6);  // 0.2 vs 0.7
}

SVJ_TEST("anchor: jumps before it was placed do not count against it") {
    Anchor anchor;
    anchor.set(0.0, 0.0, 0.0, 12);
    CHECK_EQ(anchor.jumps_since(12), 0);
    CHECK_EQ(anchor.jumps_since(14), 2);
    CHECK_EQ(anchor.jumps_since(3), 0);  // a counter reset must not go negative
}

SVJ_TEST("anchor: re-placing it clears the accumulated staleness") {
    Anchor anchor;
    anchor.set(0.0, 0.0, 0.0, 0);
    CHECK(anchor.needs_reanchor(600.0, 4));

    anchor.set(600.0, 30.0, 600.0, 4);
    CHECK(!anchor.needs_reanchor(600.0, 4));
    CHECK_NEAR(anchor.clip_position(600.0), 30.0, 1e-9);
}

SVJ_TEST("anchor: age never runs backwards on a clock that does") {
    Anchor anchor;
    anchor.set(0.0, 0.0, 100.0, 0);
    CHECK_NEAR(anchor.age_s(50.0), 0.0, 1e-9);
}

SVJ_TEST("anchor: clearing disarms it") {
    Anchor anchor;
    anchor.set(5.0, 1.0, 0.0, 0);
    anchor.clear();
    CHECK(!anchor.armed());
    CHECK(anchor.needs_reanchor(0.0, 0));
}
