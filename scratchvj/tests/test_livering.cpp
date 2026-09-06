#include <cmath>
#include <set>

#include "core/livering.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("livering: an empty ring answers nothing rather than frame zero") {
    LiveRing ring;
    ring.configure(8);
    const LivePick pick = ring.pick(1.0);
    CHECK(!pick.valid);
    CHECK(ring.empty());
    CHECK_NEAR(ring.span_s(), 0.0, 1e-12);
}

SVJ_TEST("livering: slots are handed out in turn and then reused") {
    LiveRing ring;
    ring.configure(4);
    std::set<std::size_t> seen;
    for (int i = 0; i < 4; ++i) seen.insert(ring.push(0.1 * (i + 1)));
    CHECK_EQ(seen.size(), std::size_t{4});  // four distinct slots, no overwrite
    // The fifth frame must land back on the first slot: that is the whole point
    // of a fixed ring, and the front end sizes its texture array by capacity().
    CHECK(ring.push(0.5) < 4);
    CHECK_EQ(ring.size(), std::size_t{4});
}

SVJ_TEST("livering: the newest frame is the present") {
    LiveRing ring;
    ring.configure(16);
    for (int i = 1; i <= 10; ++i) ring.push(0.05 * i);
    CHECK_NEAR(ring.newest_s(), 0.5, 1e-12);
    const LivePick pick = ring.pick(ring.newest_s());
    CHECK(pick.valid);
    CHECK(!pick.clamped);
    CHECK_NEAR(pick.capture_s, 0.5, 1e-12);
}

SVJ_TEST("livering: history stops growing once the ring is full") {
    // What an interface draws as the scratchable span. It has to stop growing,
    // or the performer is shown history the machine did not keep.
    LiveRing ring;
    ring.configure(5);
    for (int i = 1; i <= 4; ++i) ring.push(0.1 * i);
    CHECK_NEAR(ring.span_s(), 0.3, 1e-12);
    for (int i = 5; i <= 40; ++i) ring.push(0.1 * i);
    CHECK_NEAR(ring.span_s(), 0.4, 1e-12);  // four intervals, whatever else arrives
}

SVJ_TEST("livering: a frame is picked by time, not by assuming a frame rate") {
    // The fault this exists to prevent: computing a slot as newest - age * fps.
    // A live source drops frames, and that arithmetic goes wrong exactly when
    // the source is struggling -- the moment a performer is most likely to
    // notice the picture is not where their hand is.
    LiveRing ring;
    ring.configure(16);
    const double times[] = {0.00, 0.02, 0.04, 0.30, 0.32, 0.34};  // a long gap
    for (double t : times) ring.push(t);

    // 0.29 is nearest 0.30, even though even spacing would put it near 0.04.
    const LivePick pick = ring.pick(0.29);
    CHECK(pick.valid);
    CHECK(!pick.clamped);
    CHECK_NEAR(pick.capture_s, 0.30, 1e-12);
}

SVJ_TEST("livering: the nearer of two straddling frames wins") {
    LiveRing ring;
    ring.configure(8);
    ring.push(0.0);
    ring.push(1.0);
    // Rounding always downwards would bias the picture late by up to a frame
    // whenever the source stutters.
    CHECK_NEAR(ring.pick(0.4).capture_s, 0.0, 1e-12);
    CHECK_NEAR(ring.pick(0.6).capture_s, 1.0, 1e-12);
}

SVJ_TEST("livering: search stays right after the ring has wrapped") {
    // The classic ring bug: correct while filling, wrong once the oldest frame
    // no longer sits at slot zero.
    LiveRing ring;
    ring.configure(4);
    for (int i = 1; i <= 10; ++i) ring.push(static_cast<double>(i));
    CHECK_NEAR(ring.oldest_s(), 7.0, 1e-12);
    CHECK_NEAR(ring.newest_s(), 10.0, 1e-12);
    CHECK_NEAR(ring.pick(8.0).capture_s, 8.0, 1e-12);
    CHECK_NEAR(ring.pick(9.0).capture_s, 9.0, 1e-12);
}

SVJ_TEST("livering: scratching past the tail says so instead of freezing") {
    // A frozen picture with no explanation reads as a crash. The limit is real
    // -- the machine cannot scratch what it did not keep -- so it is reported.
    LiveRing ring;
    ring.configure(4);
    for (int i = 1; i <= 6; ++i) ring.push(static_cast<double>(i));

    const LivePick old = ring.pick(1.0);  // long gone
    CHECK(old.valid);
    CHECK(old.clamped);
    CHECK_NEAR(old.capture_s, ring.oldest_s(), 1e-12);

    const LivePick future = ring.pick(99.0);  // ahead of the present
    CHECK(future.valid);
    CHECK(future.clamped);
    CHECK_NEAR(future.capture_s, ring.newest_s(), 1e-12);

    // Exactly the oldest frame held is a request the ring CAN answer, so it is
    // not a clamp. Off-by-one here would light the edge warning permanently.
    CHECK(!ring.pick(ring.oldest_s()).clamped);
}

SVJ_TEST("livering: a source that restarts begins a new history") {
    // A capture clock that jumps backwards -- a sender restarting, a device
    // reconnecting -- leaves times that do not increase, and a binary search
    // over those returns nonsense rather than failing. Starting again is the
    // only answer that stays truthful.
    LiveRing ring;
    ring.configure(8);
    for (int i = 1; i <= 6; ++i) ring.push(static_cast<double>(i) * 0.1);
    CHECK_EQ(ring.size(), std::size_t{6});

    ring.push(0.0);  // the sender came back
    CHECK_EQ(ring.size(), std::size_t{1});
    CHECK_NEAR(ring.newest_s(), 0.0, 1e-12);
    CHECK_NEAR(ring.span_s(), 0.0, 1e-12);
}

SVJ_TEST("livering: every frame it claims to hold can be picked back out") {
    // The property that matters to a performer: what the ring says it has is
    // what a hand can reach.
    LiveRing ring;
    ring.configure(32);
    for (int i = 1; i <= 100; ++i) ring.push(static_cast<double>(i) * 0.033);

    CHECK_EQ(ring.size(), std::size_t{32});
    std::set<std::size_t> reachable;
    for (int i = 0; i < 400; ++i) {
        const double t = ring.oldest_s() +
                         ring.span_s() * static_cast<double>(i) / 399.0;
        const LivePick pick = ring.pick(t);
        CHECK(pick.valid);
        CHECK(!pick.clamped);
        reachable.insert(pick.slot);
    }
    CHECK_EQ(reachable.size(), ring.size());
}
