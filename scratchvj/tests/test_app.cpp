#include "app/dashboard.h"
#include "app/simulation.h"
#include "harness.h"

using namespace svj;

namespace {

// Runs the scripted performance through the same chain the demo uses, and reports
// what happened. This is what keeps the demo honest: if the script stops
// exercising a dropout or a backspin, a test fails rather than the demo quietly
// becoming a pretty animation.
struct Outcome {
    bool saw_lost_link = false;
    bool saw_backspin = false;
    bool saw_reverse = false;
    bool saw_fast_scratch = false;
    bool position_moved_during_dropout = false;
    int jumps = 0;
    double frozen_position = 0.0;
};

Outcome run(double seconds, double fps) {
    Surface surface;
    Simulation simulation;
    simulation.configure(surface);

    TimecodeConfig config;
    config.profile = SignalProfile::Wireless;
    config.mode = TransportMode::Relative;
    TimecodeTracker tracker(config);
    GestureTracker gestures;

    Outcome outcome;
    bool inside_dropout = false;
    double dropout_start_position = 0.0;

    const double dt = 1.0 / fps;
    for (double t = 0.0; t < seconds; t += dt) {
        simulation.step(t, surface, static_cast<std::uint64_t>(t * 1e6));
        const TimecodeState& state = tracker.submit(simulation.deck_a());
        gestures.update(t, state.velocity, state.confidence);

        if (state.link == LinkState::Lost) {
            outcome.saw_lost_link = true;
            if (!inside_dropout) {
                inside_dropout = true;
                dropout_start_position = state.position_s;
            } else if (state.position_s != dropout_start_position) {
                outcome.position_moved_during_dropout = true;
            }
            outcome.frozen_position = state.position_s;
        } else {
            inside_dropout = false;
        }

        if (gestures.backspin()) outcome.saw_backspin = true;
        if (state.velocity < -0.5f) outcome.saw_reverse = true;
        if (gestures.scratch_rate() > 3.0f) outcome.saw_fast_scratch = true;
    }
    outcome.jumps = tracker.jump_count();
    return outcome;
}

}  // namespace

SVJ_TEST("app: the script exercises the moments worth watching") {
    const Outcome outcome = run(24.0, 60.0);
    CHECK(outcome.saw_lost_link);
    CHECK(outcome.saw_backspin);
    CHECK(outcome.saw_reverse);
    CHECK(outcome.saw_fast_scratch);
}

SVJ_TEST("app: A LOST LINK FREEZES THE POSITION, it does not drift") {
    // The behaviour the whole wireless profile exists for, checked end to end.
    const Outcome outcome = run(24.0, 60.0);
    CHECK(outcome.saw_lost_link);
    CHECK(!outcome.position_moved_during_dropout);
}

SVJ_TEST("app: a legitimate performance produces no false timecode jumps") {
    // Sharp speed changes used to be read as discontinuities, which then aged the
    // follower-mode anchor for no reason. Checked across several block rates,
    // since the jump budget scales with the block length.
    for (const double fps : {30.0, 60.0, 120.0, 375.0}) {
        const Outcome outcome = run(24.0, fps);
        CHECK_EQ(outcome.jumps, 0);
    }
}

SVJ_TEST("app: the script is deterministic") {
    const Outcome first = run(24.0, 60.0);
    const Outcome second = run(24.0, 60.0);
    CHECK_EQ(first.jumps, second.jumps);
    CHECK_NEAR(first.frozen_position, second.frozen_position, 1e-12);
}

SVJ_TEST("app: untouched controls stay unknown for the whole run") {
    // ch2's knobs are never moved by the script, so they must still read as
    // unknown at the end rather than as sitting at zero.
    Surface surface;
    Simulation simulation;
    simulation.configure(surface);
    for (double t = 0.0; t < 24.0; t += 1.0 / 60.0) {
        simulation.step(t, surface, static_cast<std::uint64_t>(t * 1e6));
    }
    CHECK(!surface.at(surface.find("ch2.eq.hi")).known);
    CHECK(!surface.at(surface.find("ch2.filter")).known);
    CHECK(surface.at(surface.find("xfader")).known);
}

SVJ_TEST("dashboard: a frame renders without ansi and names what it shows") {
    Surface surface;
    Simulation simulation;
    simulation.configure(surface);
    simulation.step(2.0, surface, 2000000);

    TimecodeTracker tracker;
    tracker.submit(simulation.deck_a());
    GestureTracker gestures;
    Transport transport;
    transport.configure(240.0, BeatGrid{124.0, 0.0});
    transport.map(12.0);

    CacheHeader clip;
    clip.width = 1920; clip.height = 1080; clip.fps_num = 60; clip.fps_den = 1;
    clip.frame_count = 14400;
    FrameWindow window;
    window.configure(clip.frame_count,
                     block_bytes_per_frame(clip.width, clip.height, BlockFormat::BC1),
                     WindowConfig{});
    window.update(clip.frame_at(12.0), 1.0f);

    MappingEngine mapping;
    DashboardView view;
    view.phase = "essai";
    DeckView a{"DECK A", &tracker.state(), &gestures, &transport, &window, &clip};
    DeckView b{"DECK B", &tracker.state(), &gestures, &transport, &window, &clip};

    const std::string frame = render_dashboard(view, a, b, surface, mapping, false);
    CHECK(frame.find("scratchvj") != std::string::npos);
    CHECK(frame.find("DECK A") != std::string::npos);
    CHECK(frame.find("SURFACE") != std::string::npos);
    CHECK(frame.find("essai") != std::string::npos);
    CHECK(frame.find("\033[") == std::string::npos);  // no escape codes in plain mode
}

SVJ_TEST("dashboard: an unknown control is drawn as unknown, never as zero") {
    Surface surface;
    surface.declare("never.touched", ControlKind::Knob);
    TimecodeTracker tracker;
    MappingEngine mapping;
    DashboardView view;
    DeckView empty;

    const std::string frame = render_dashboard(view, empty, empty, surface, mapping, false);
    CHECK(frame.find("never.touched") != std::string::npos);
    CHECK(frame.find("........................") != std::string::npos);
    CHECK(frame.find("?") != std::string::npos);
}

SVJ_TEST("dashboard: ansi mode emits colour and repositions the cursor") {
    Surface surface;
    TimecodeTracker tracker;
    MappingEngine mapping;
    DashboardView view;
    DeckView empty;
    const std::string frame = render_dashboard(view, empty, empty, surface, mapping, true);
    CHECK(frame.rfind("\033[H\033[J", 0) == 0);
}

SVJ_TEST("app: the scripted platter position is continuous everywhere") {
    // A hand cannot teleport. Checked directly rather than only through the jump
    // detector, so a future edit to the script fails here with a clear reason.
    Surface surface;
    Simulation simulation;
    simulation.configure(surface);

    const double dt = 1.0 / 600.0;
    double previous = 0.0;
    bool first = true;
    for (double t = 0.0; t < 24.0; t += dt) {
        simulation.step(t, surface, static_cast<std::uint64_t>(t * 1e6));
        const DecoderSample sample = simulation.deck_a();
        if (sample.signal_level <= 0.0f) {   // the scripted dropout
            first = true;
            continue;
        }
        if (!first) {
            // 24x nominal speed is far beyond any real backspin.
            CHECK(std::fabs(sample.position_s - previous) < 24.0 * dt);
        }
        previous = sample.position_s;
        first = false;
    }
}
