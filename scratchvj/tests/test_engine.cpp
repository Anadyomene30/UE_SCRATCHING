#include "app/engine.h"
#include "app/simulation.h"
#include "harness.h"

using namespace svj;

namespace {

// The demo, exactly as `scratchvj demo` runs it. Nothing here re-implements the
// wiring: if these tests pass, the command does the same thing, because it is the
// same object being driven by the same script.
struct Rig {
    Engine engine;
    Simulation simulation;

    explicit Rig(double bpm = 124.0) {
        engine.configure(bpm);
        simulation.configure(engine.surface());
        engine.bind();
    }

    void run(double seconds, double fps, const std::function<void(double)>& observe = {}) {
        const double dt = 1.0 / fps;
        for (double t = 0.0; t < seconds; t += dt) {
            const auto now_us = static_cast<std::uint64_t>(t * 1e6);
            simulation.step(t, engine.surface(), now_us);

            EngineFrame frame;
            frame.time_s = t;
            frame.dt_s = static_cast<float>(dt);
            frame.now_us = now_us;
            frame.deck_a = simulation.deck_a();
            frame.deck_b = simulation.deck_b();

            const SimEvent& e = simulation.events();
            frame.commands_a.loop_in = e.loop_in;
            frame.commands_a.loop_exit = e.loop_exit;
            frame.commands_a.slip_on = e.slip_on;
            frame.commands_a.slip_off = e.slip_off;
            frame.commands_a.cue_jump = e.cue_jump;
            frame.commands_a.cue_index = e.cue_index;

            engine.step(frame);
            if (observe) observe(t);
        }
    }
};

// A deck that has lost its link, without needing the script to reach the dropout.
DecoderSample lost(double t_s) {
    DecoderSample sample;
    sample.time_s = t_s;
    sample.position_s = -1.0;
    sample.pitch = 0.0f;
    sample.signal_level = 0.0f;
    return sample;
}

DecoderSample moving(double t_s, double position_s, float pitch) {
    DecoderSample sample;
    sample.time_s = t_s;
    sample.position_s = position_s;
    sample.pitch = pitch;
    sample.signal_level = 1.0f;
    return sample;
}

}  // namespace

SVJ_TEST("engine: the surface is declared by its owner, and mappings bind to it after") {
    // configure() cannot know what controls exist -- a learned Elite and a
    // recorded take declare different ones -- so binding is a separate step, and
    // it must report what it could not find rather than dropping it in silence.
    Engine engine;
    engine.configure(124.0);
    CHECK(engine.bind().size() > 0);  // nothing declared yet: every id is unresolved

    Simulation simulation;
    simulation.configure(engine.surface());
    CHECK_EQ(engine.bind().size(), static_cast<std::size_t>(0));
}

SVJ_TEST("engine: a full run of the script reaches every mapped destination") {
    Rig rig;
    std::vector<bool> ever_active(rig.engine.mapping().size(), false);
    rig.run(24.0, 60.0, [&](double) {
        for (std::size_t i = 0; i < rig.engine.mapping().size(); ++i) {
            if (rig.engine.mapping().active(i)) ever_active[i] = true;
        }
    });
    for (std::size_t i = 0; i < ever_active.size(); ++i) {
        CHECK(ever_active[i]);
    }
}

SVJ_TEST("engine: the modulators follow the platter, backwards included") {
    // The LFO is phased off the played position, not off wall-clock time. During
    // the backspin the beat position must run backwards -- that is the whole
    // reason the modulators are updated after the transport and not before it.
    Rig rig;
    double before = 0.0, during = 0.0;
    rig.run(9.0, 120.0, [&](double t) {
        if (t >= 7.4 && t < 7.6) before = rig.engine.deck_a().transport.position_s();
        if (t >= 8.4 && t < 8.6) during = rig.engine.deck_a().transport.position_s();
    });
    CHECK(during < before);
}

SVJ_TEST("engine: a loop is set where the platter was, not where it has reached") {
    // The command is applied before advance(), so a loop dropped on the beat is
    // quantised from the position the button was pressed at. A frame's worth of
    // drift is a loop that starts a beat late whenever the press lands near a
    // boundary -- which, on a loop dropped on the beat, is every time.
    //
    // Run at two frames a second so a frame spans more than a beat: at 60fps both
    // candidate positions snap to the same boundary and the test would pass
    // whichever one the engine used.
    const BeatGrid grid{124.0, 0.0};
    const double dt = 0.5;
    CHECK(dt > grid.beat_duration_s());

    Engine engine;
    engine.configure(grid.bpm);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = static_cast<float>(dt);
    for (int i = 0; i < 4; ++i) {
        frame.time_s = i * dt;
        frame.deck_a = moving(frame.time_s, 10.0 + frame.time_s, 1.0f);
        frame.deck_b = moving(frame.time_s, 0.0, 0.0f);
        engine.step(frame);
    }
    const double at_press = engine.deck_a().transport.position_s();

    frame.time_s = 4 * dt;
    frame.deck_a = moving(frame.time_s, 10.0 + frame.time_s, 1.0f);
    frame.commands_a.loop_in = true;
    frame.commands_a.loop_seconds = 2.0;
    engine.step(frame);

    const double reached = engine.deck_a().transport.shadow_position_s();
    CHECK(engine.deck_a().transport.loop().active);
    CHECK_NEAR(engine.deck_a().transport.loop().start_s, grid.snap(at_press), 1e-9);
    CHECK(grid.snap(at_press) != grid.snap(reached));  // otherwise the check is vacuous
}

SVJ_TEST("engine: HOLD keeps the last motion through a dropout") {
    // A wireless dropout is not a stopped platter. Under Hold, whatever the
    // effects were doing carries on rather than snapping, so a stutter in the
    // radio is not a visible event.
    Engine engine;
    engine.configure(124.0);
    engine.bind();
    engine.set_link_loss_policy(LinkLossPolicy::Hold);

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i) {
        frame.time_s = i / 60.0;
        frame.deck_a = moving(frame.time_s, 10.0 + frame.time_s * 2.0, 2.0f);
        frame.deck_b = moving(frame.time_s, 0.0, 0.0f);
        engine.step(frame);
    }
    const float locked = engine.packet(0, 0).deck_a.velocity;
    CHECK(locked > 0.5f);

    for (int i = 60; i < 90; ++i) {
        frame.time_s = i / 60.0;
        frame.deck_a = lost(frame.time_s);
        engine.step(frame);
    }
    CHECK(engine.deck_a().timecode.state().link == LinkState::Lost);
    CHECK_NEAR(engine.packet(0, 0).deck_a.velocity, locked, 1e-6);
}

SVJ_TEST("engine: ZERO snaps motion to rest during a dropout") {
    Engine engine;
    engine.configure(124.0);
    engine.bind();
    engine.set_link_loss_policy(LinkLossPolicy::Zero);

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i) {
        frame.time_s = i / 60.0;
        frame.deck_a = moving(frame.time_s, 10.0 + frame.time_s * 2.0, 2.0f);
        frame.deck_b = moving(frame.time_s, 0.0, 0.0f);
        engine.step(frame);
    }
    CHECK(engine.packet(0, 0).deck_a.velocity > 0.5f);

    frame.time_s = 1.0;
    frame.deck_a = lost(1.0);
    engine.step(frame);
    CHECK_EQ(engine.packet(0, 0).deck_a.velocity, 0.0f);
}

SVJ_TEST("engine: the policy never touches a deck whose link is good") {
    // Every policy must be identical while locked; they differ only in the fault.
    const auto velocity_after_a_second = [](LinkLossPolicy policy) {
        Engine engine;
        engine.configure(124.0);
        engine.bind();
        engine.set_link_loss_policy(policy);
        EngineFrame frame;
        frame.dt_s = 1.0f / 60.0f;
        for (int i = 0; i < 60; ++i) {
            frame.time_s = i / 60.0;
            frame.deck_a = moving(frame.time_s, 10.0 + frame.time_s * 2.0, 2.0f);
            frame.deck_b = moving(frame.time_s, 0.0, 0.0f);
            engine.step(frame);
        }
        return engine.packet(0, 0).deck_a.velocity;
    };
    const float hold = velocity_after_a_second(LinkLossPolicy::Hold);
    CHECK_NEAR(velocity_after_a_second(LinkLossPolicy::Zero), hold, 1e-9);
    CHECK_NEAR(velocity_after_a_second(LinkLossPolicy::Decay), hold, 1e-9);
}

SVJ_TEST("engine: the packet says the link is down, whatever the motion reads") {
    // The receiver must be able to tell a held value from a measured one, or it
    // cannot draw the difference between a hand on the platter and a dead radio.
    Rig rig;
    bool saw_holding = false;
    rig.run(24.0, 60.0, [&](double t) {
        const StatePacket packet = rig.engine.packet(static_cast<std::uint64_t>(t * 1e6), 0);
        if (packet.gesture_bits & kGestureHoldingA) saw_holding = true;
    });
    CHECK(saw_holding);
}

SVJ_TEST("engine: the schema matches the surface it was taken from") {
    Rig rig;
    const SchemaPacket schema = rig.engine.schema();
    CHECK_EQ(schema.entries.size(), rig.engine.surface().size());
    CHECK_EQ(schema.schema_hash, svj::schema_hash(schema.entries));
    CHECK_EQ(rig.engine.packet(0, schema.schema_hash).values.size(),
             rig.engine.surface().size());
}

SVJ_TEST("engine: an unknown control is still unknown on the wire") {
    // ch2's knobs are never moved by the script. They must travel as unknown, so
    // the far end can draw them as unknown rather than as sitting at zero.
    Rig rig;
    rig.run(24.0, 60.0);
    const StatePacket packet = rig.engine.packet(0, 0);
    const ControlIndex untouched = rig.engine.surface().find("ch2.eq.hi");
    const ControlIndex touched = rig.engine.surface().find("xfader");
    CHECK(!packet.known[static_cast<std::size_t>(untouched)]);
    CHECK(packet.known[static_cast<std::size_t>(touched)]);
}

SVJ_TEST("engine: two runs of the same script produce the same wire packet") {
    // The engine holds smoothing filters and envelope followers, so this is a
    // stronger claim than the simulation being deterministic.
    const auto final_packet = [] {
        Rig rig;
        rig.run(24.0, 60.0);
        return rig.engine.packet(0, 0);
    };
    const StatePacket first = final_packet();
    const StatePacket second = final_packet();
    CHECK_EQ(first.gesture_bits, second.gesture_bits);
    CHECK_NEAR(first.deck_a.pos_s, second.deck_a.pos_s, 1e-9);
    CHECK_NEAR(first.deck_a.scratch_rate, second.deck_a.scratch_rate, 1e-9);
    for (std::size_t i = 0; i < first.values.size(); ++i) {
        CHECK_NEAR(first.values[i], second.values[i], 1e-9);
    }
}
