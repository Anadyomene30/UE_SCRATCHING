#include "core/framewindow.h"
#include "core/videocache.h"
#include "harness.h"

using namespace svj;

namespace {

// A window of exactly `capacity` frames over `frames` frames.
FrameWindow sized(std::uint32_t frames, std::uint32_t capacity, std::uint32_t margin = 24) {
    WindowConfig config;
    config.budget_bytes = static_cast<std::uint64_t>(capacity) * 1000;
    config.edge_margin = margin;
    FrameWindow window;
    window.configure(frames, 1000, config);
    return window;
}

}  // namespace

SVJ_TEST("window: capacity comes from the budget, not from a duration") {
    // The same budget holds far more small frames than large ones, which is the
    // whole reason the window is sized this way.
    WindowConfig config;
    config.budget_bytes = 1ull << 30;

    FrameWindow hd;
    hd.configure(100000, block_bytes_per_frame(1280, 720, BlockFormat::BC1), config);

    FrameWindow equirect;
    equirect.configure(100000, block_bytes_per_frame(3840, 1920, BlockFormat::BC1), config);

    CHECK(hd.capacity() > equirect.capacity() * 6);
    CHECK(hd.capacity() > 2000);
    CHECK(equirect.capacity() > 250);
}

SVJ_TEST("window: a clip that fits entirely never slides") {
    FrameWindow window = sized(200, 500);
    CHECK(window.holds_whole_clip());

    CHECK(window.update(0, 1.0f));  // first placement
    CHECK_EQ(window.resident().first, std::uint32_t{0});
    CHECK_EQ(window.resident().count, std::uint32_t{200});

    for (std::uint32_t f = 0; f < 200; ++f) {
        CHECK(!window.update(f, 1.0f));
    }
}

SVJ_TEST("window: a frame larger than the budget still leaves the clip playable") {
    WindowConfig config;
    config.budget_bytes = 10;
    FrameWindow window;
    window.configure(50, 1000, config);
    CHECK_EQ(window.capacity(), std::uint32_t{1});
    CHECK(window.update(10, 1.0f));
    CHECK(window.contains(10));
}

SVJ_TEST("window: playing forward biases the window ahead of the playhead") {
    FrameWindow window = sized(10000, 1000);
    window.update(5000, 1.0f);
    const FrameRange r = window.resident();
    const std::uint32_t ahead = r.last() - 5000;
    const std::uint32_t behind = 5000 - r.first;
    CHECK(ahead > behind);
}

SVJ_TEST("window: playing backwards biases it behind") {
    FrameWindow window = sized(10000, 1000);
    window.update(5000, -1.0f);
    const FrameRange r = window.resident();
    CHECK(5000 - r.first > r.last() - 5000);
}

SVJ_TEST("window: a standstill sits centred, leaving room either way") {
    FrameWindow window = sized(10000, 1000);
    window.update(5000, 0.0f);
    const FrameRange r = window.resident();
    const int ahead = static_cast<int>(r.last()) - 5000;
    const int behind = 5000 - static_cast<int>(r.first);
    CHECK(std::abs(ahead - behind) < 4);
}

SVJ_TEST("window: SCRATCHING MID-WINDOW NEVER TRIGGERS A REFILL") {
    // The case that must never stutter. Working back and forth in the middle of
    // the window changes direction constantly; if direction alone recentred the
    // window, every scratch would thrash the disk.
    FrameWindow window = sized(10000, 1000);
    window.update(5000, 1.0f);

    int refills = 0;
    for (int i = 0; i < 400; ++i) {
        const float velocity = (i % 2 == 0) ? -4.0f : 4.0f;
        const std::uint32_t playhead = 5000 + static_cast<std::uint32_t>((i % 7) * 3);
        if (window.update(playhead, velocity)) ++refills;
    }
    CHECK_EQ(refills, 0);
}

SVJ_TEST("window: approaching an edge slides it, fetching only the new end") {
    FrameWindow window = sized(10000, 1000, 24);
    window.update(5000, 1.0f);
    const FrameRange before = window.resident();

    // Walk forward until it slides.
    std::uint32_t playhead = 5000;
    bool slid = false;
    while (playhead < 9000 && !slid) {
        playhead += 10;
        slid = window.update(playhead, 1.0f);
    }
    CHECK(slid);
    CHECK(!window.reloaded_everything());       // it is a slide, not a reload
    CHECK(window.pending_load().count > 0);
    CHECK(window.pending_load().count < window.capacity());
    CHECK(window.resident().first > before.first);
    CHECK(window.contains(playhead));
}

SVJ_TEST("window: a needle drop outside the window reloads the whole thing") {
    // The brief load the plan admits to, and the only time the user should feel one.
    FrameWindow window = sized(10000, 1000);
    window.update(500, 1.0f);
    CHECK(window.update(9000, 1.0f));
    CHECK(window.reloaded_everything());
    CHECK_EQ(window.pending_load().count, window.capacity());
    CHECK(window.contains(9000));
}

SVJ_TEST("window: it never runs off either end of the clip") {
    FrameWindow window = sized(1000, 300);

    window.update(0, -1.0f);
    CHECK_EQ(window.resident().first, std::uint32_t{0});
    CHECK_EQ(window.resident().count, std::uint32_t{300});

    window.update(999, 1.0f);
    CHECK_EQ(window.resident().last(), std::uint32_t{999});
    CHECK_EQ(window.resident().count, std::uint32_t{300});
}

SVJ_TEST("window: sitting at the clip start does not slide forever") {
    // The playhead is inside the edge margin, but there is nowhere to slide to.
    FrameWindow window = sized(1000, 300, 24);
    window.update(2, 1.0f);
    int moves = 0;
    for (int i = 0; i < 50; ++i) {
        if (window.update(2, 1.0f)) ++moves;
    }
    CHECK_EQ(moves, 0);
}

SVJ_TEST("window: a playhead past the last frame is clamped, not read out of range") {
    FrameWindow window = sized(100, 40);
    window.update(1'000'000, 1.0f);
    CHECK(window.resident().last() <= 99);
    CHECK(window.contains(99));
}

SVJ_TEST("window: the reported span matches what the filmstrip should draw") {
    FrameWindow window = sized(10000, 600);
    window.update(5000, 1.0f);
    CHECK_NEAR(window.window_seconds(1.0 / 60.0), 10.0, 1e-9);
}

SVJ_TEST("window: an empty clip is inert rather than crashing") {
    FrameWindow window;
    WindowConfig config;
    window.configure(0, 0, config);
    CHECK_EQ(window.capacity(), std::uint32_t{0});
    CHECK(!window.update(0, 1.0f));
    CHECK(!window.holds_whole_clip());
}

SVJ_TEST("window: reconfiguring forgets the old placement") {
    FrameWindow window = sized(10000, 1000);
    window.update(5000, 1.0f);
    CHECK(window.resident().count > 0);

    window.configure(200, 1000, WindowConfig{});
    CHECK_EQ(window.resident().count, std::uint32_t{0});
    CHECK(window.update(10, 1.0f));  // places afresh
}
