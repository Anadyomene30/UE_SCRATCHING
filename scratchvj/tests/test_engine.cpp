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
    sample.locked = true;
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

SVJ_TEST("engine: the script's kick reaches a mapping through the spectrum") {
    // The whole reactive path in one test: synthetic audio -> the analyser ->
    // Engine's `bands` pointer -> the mapping engine. Each link is covered on
    // its own elsewhere; this is the only thing that proves they are connected,
    // and a mistyped pointer here would leave every AudioBand mapping silently
    // reading zero forever.
    Rig rig;

    Mapping m;
    m.name = "graves -> test";
    m.source.kind = SourceKind::AudioBand;
    m.source.index = 0;  // the bottom band, where the 55 Hz kick lives
    const std::size_t index = rig.engine.mapping().add(m);
    rig.engine.bind();

    float peak = 0.0f;
    const double dt = 1.0 / 60.0;
    std::vector<float> block(2048);
    for (double t = 0.0; t < 2.0; t += dt) {
        const std::size_t written =
            rig.simulation.audio(t, t + dt, rig.engine.bpm(), 48000.0, block.data(),
                                 block.size());
        rig.engine.analyse_audio(block.data(), written);

        const auto now_us = static_cast<std::uint64_t>(t * 1e6);
        rig.simulation.step(t, rig.engine.surface(), now_us);
        EngineFrame frame;
        frame.time_s = t;
        frame.dt_s = static_cast<float>(dt);
        frame.now_us = now_us;
        frame.deck_a = rig.simulation.deck_a();
        frame.deck_b = rig.simulation.deck_b();
        rig.engine.step(frame);

        peak = std::max(peak, rig.engine.mapping().value(index));
    }
    CHECK(peak > 0.3f);
}

SVJ_TEST("engine: the bass band pumps on the beat instead of sitting pinned") {
    // A band that never comes back down is not reactive, it is a lamp. The kick
    // rings for about 35 ms out of every 484 ms at 124 BPM, so band 0 has to
    // spend most of the beat well below its peak -- otherwise anything mapped to
    // it is effectively a constant.
    Rig rig;
    std::vector<float> block(4096);
    float low = 1.0f;
    float high = 0.0f;

    const double dt = 1.0 / 120.0;
    for (double t = 0.0; t < 2.0; t += dt) {
        const std::size_t written =
            rig.simulation.audio(t, t + dt, rig.engine.bpm(), 48000.0, block.data(),
                                 block.size());
        rig.engine.analyse_audio(block.data(), written);
        if (t < 1.0) continue;  // let the follower settle before measuring
        const float band = rig.engine.spectrum().bands()[0];
        low = std::min(low, band);
        high = std::max(high, band);
    }
    CHECK(high > 0.4f);
    CHECK(low < high * 0.5f);
}

SVJ_TEST("engine: the bands fall back to zero when the audio stops") {
    // A disconnected input must not pin the video on the last sound it heard.
    Rig rig;
    std::vector<float> block(2048);
    const std::size_t written =
        rig.simulation.audio(0.0, 0.25, rig.engine.bpm(), 48000.0, block.data(),
                             block.size());
    rig.engine.analyse_audio(block.data(), written);
    CHECK(rig.engine.spectrum().bands()[0] > 0.2f);

    rig.run(3.0, 60.0);  // three seconds of the script, and no audio at all
    CHECK(rig.engine.spectrum().bands()[0] < 0.02f);
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

// --- the third layer ---------------------------------------------------------

SVJ_TEST("engine: THE OVERLAY ADVANCES WITH NO PLATTER AT ALL") {
    // The layer has no timecode source by construction. If it only moved when a
    // deck moved, it would not be a layer, it would be a third deck waiting for
    // a turntable that does not exist.
    Engine engine;
    engine.configure(120.0);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    for (int i = 1; i <= 60; ++i) {
        frame.time_s = i / 60.0;
        frame.now_us = static_cast<std::uint64_t>(frame.time_s * 1e6);
        engine.step(frame);
    }

    // Nothing was ever fed to a platter, and the layer has still moved.
    CHECK(engine.overlay().played.position_s > 0.0);
    CHECK(engine.overlay().played.velocity != 0.0);
}

SVJ_TEST("engine: the overlay bounces instead of wrapping") {
    // Eight seconds of texture at four beats a pass, 120 bpm: one pass is two
    // seconds, so three seconds in it is on the way back.
    Engine engine;
    engine.configure(120.0);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    for (int i = 1; i <= 180; ++i) {
        frame.time_s = i / 60.0;
        frame.now_us = static_cast<std::uint64_t>(frame.time_s * 1e6);
        engine.step(frame);
    }

    CHECK(engine.overlay().played.reversed);
    CHECK(engine.overlay().played.velocity < 0.0);
    // And it stayed inside its own clip the whole way.
    CHECK(engine.overlay().played.position_s >= 0.0);
    CHECK(engine.overlay().played.position_s <= engine.overlay().clip.duration_s());
}

SVJ_TEST("engine: loading a clip on the overlay keeps it tempo-locked and bouncing") {
    // The bug this pins: Deck::load rebuilt the clock, and DeckClock::configure
    // resets source, mode and takeover to a scratch deck's defaults. Dropping a
    // logo on the overlay therefore turned it into a looping timecode deck
    // that grabbed the left platter -- the mask followed the hand.
    Engine engine;
    engine.configure(120.0);
    CHECK(engine.overlay().clock.source() == DeckSource::TempoLocked);
    CHECK(engine.overlay().clock.mode() == ClipPlayMode::PingPong);
    CHECK(engine.overlay().clock.takeover() == TakeoverMode::Ignore);
    const double beats = engine.overlay().clock.beats_per_cycle();
    CHECK(beats > 0.0);

    CacheHeader logo;
    logo.width = 640;
    logo.height = 360;
    logo.fps_num = 30;
    logo.fps_den = 1;
    logo.frame_count = 90;
    logo.format = BlockFormat::BC3;
    logo.flags = kCacheAlpha;
    engine.overlay().load(logo, "logo", 120.0);

    CHECK(engine.overlay().clock.source() == DeckSource::TempoLocked);
    CHECK(engine.overlay().clock.mode() == ClipPlayMode::PingPong);
    CHECK(engine.overlay().clock.takeover() == TakeoverMode::Ignore);
    CHECK_NEAR(engine.overlay().clock.beats_per_cycle(), beats, 1e-12);
    CHECK_NEAR(engine.overlay().clip.duration_s(), 3.0, 1e-9);

    // And it still bounces over the NEW duration rather than the old one.
    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    bool reversed = false;
    for (int i = 1; i <= 600; ++i) {
        frame.time_s = i / 60.0;
        frame.now_us = static_cast<std::uint64_t>(frame.time_s * 1e6);
        engine.step(frame);
        reversed = reversed || engine.overlay().played.reversed;
        CHECK(engine.overlay().played.position_s <= 3.0 + 1e-9);
    }
    CHECK(reversed);
}

SVJ_TEST("engine: a fabricated 2:1 deck carries the equirect flag a real cache would") {
    // The interface asks the flag, not the aspect ratio, so the demo's 360 deck
    // must say so the same way an analysed clip does.
    Engine engine;
    engine.configure(120.0);
    CHECK(engine.deck_a().clip.is_equirect());
    CHECK(!engine.deck_b().clip.is_equirect());
}

SVJ_TEST("engine: the crossfader moves the decks and leaves the overlay alone") {
    Engine engine;
    engine.configure(120.0);
    const ControlIndex xfader = engine.surface().declare("xfader", ControlKind::Fader);
    const ControlIndex fa = engine.surface().declare("ch1.fader", ControlKind::Fader);
    const ControlIndex fb = engine.surface().declare("ch2.fader", ControlKind::Fader);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;

    float seen = -1.0f;
    for (float x = 0.0f; x <= 1.0f; x += 0.25f) {
        frame.now_us += 16666;
        engine.surface().set(xfader, x, frame.now_us);
        engine.surface().set(fa, 1.0f, frame.now_us);
        engine.surface().set(fb, 1.0f, frame.now_us);
        frame.time_s += frame.dt_s;
        engine.step(frame);

        if (seen < 0.0f) seen = engine.stack().overlay;
        CHECK_NEAR(engine.stack().overlay, seen, 1e-6);
    }
    CHECK(seen > 0.0f);  // and it was actually showing, not merely constant at zero
}

SVJ_TEST("engine: the eq knobs steer the 360 gaze through the mapping") {
    // The gaze must come out of the mapping engine, not straight off the knob:
    // that is what lets an LFO or a headset drive it through the same door. And
    // an untouched knob must leave the gaze alone -- a ghost control snapping
    // the view to zero at startup would be a fabricated reading acted upon.
    Engine engine;
    engine.configure(124.0);
    Simulation simulation;
    simulation.configure(engine.surface());
    engine.bind();

    const double before = engine.view_a().yaw_deg;

    EngineFrame frame;
    frame.time_s = 0.5;
    frame.dt_s = 1.0f / 60.0f;
    engine.step(frame);
    const double untouched = engine.view_a().yaw_deg;
    CHECK_NEAR(untouched, before, 1e-9);  // nobody moved anything

    const ControlIndex knob = engine.surface().find("ch1.eq.hi");
    CHECK(knob != kNoControl);
    engine.surface().set(knob, 0.75f, 1000);
    // The mapping smooths its outputs, so the gaze eases to the knob rather
    // than teleporting; two simulated seconds is ample for it to settle.
    for (int i = 0; i < 120; ++i) {
        frame.time_s = 0.6 + i / 60.0;
        engine.step(frame);
    }
    // The rig maps the knob over -180..180 (a centre detent looks straight
    // ahead), so three quarters of the throw is +90 degrees.
    CHECK_NEAR(engine.view_a().yaw_deg, -180.0 + 0.75 * 360.0, 1.0);
}

SVJ_TEST("engine: a mapping to an unknown destination is reported at bind, not ignored") {
    Engine engine;
    engine.configure(120.0);
    Mapping m;
    m.name = "typo";
    m.source.kind = SourceKind::DeckVelocity;
    m.destination.target = "fx.a.glitchh.amount";
    engine.mapping().add(m);
    Simulation simulation;
    simulation.configure(engine.surface());
    const auto unknown = engine.bind();
    bool named = false;
    for (const std::string& u : unknown) named = named || u == "fx.a.glitchh.amount";
    CHECK(named);
}

SVJ_TEST("engine: every destination the demo maps is known to the registry") {
    // The demo's own mappings used to be half decoration: only four of its
    // targets were dispatched. Now every local one must resolve.
    Engine engine;
    engine.configure(120.0);
    Simulation simulation;
    simulation.configure(engine.surface());
    for (const std::string& u : engine.bind()) {
        CHECK(u.empty());  // prints the offender
    }
}

SVJ_TEST("engine: a pad mapped to a cue jumps on the press edge and not while held") {
    Engine engine;
    engine.configure(120.0);
    const ControlIndex pad = engine.surface().declare("pad.elite.a.3", ControlKind::Pad);
    Mapping m;
    m.name = "pad 3 -> cue 3";
    m.source.kind = SourceKind::Control;
    m.source.control_id = "pad.elite.a.3";
    m.destination.target = "deck.a.cue.3";
    engine.mapping().add(m);
    engine.bind();
    // Cue 3 (index 2) sits at 96 s in the demo deck.
    engine.deck_a().transport.set_cue(2, 96.0, 3);

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    auto step = [&](float pad_value) {
        frame.time_s += frame.dt_s;
        frame.now_us += 16666;
        engine.surface().set(pad, pad_value, frame.now_us);
        engine.step(frame);
    };
    step(0.0f);
    step(1.0f);  // press: the jump happens this step
    const double after_press = engine.deck_a().transport.position_s();
    CHECK_NEAR(after_press, 96.0, 0.5);
    // Held: no further jump. Move the platter on and see it stay moved.
    engine.deck_a().transport.jump_to_cue(0);
    step(1.0f);
    step(1.0f);
    CHECK(engine.deck_a().transport.position_s() < 10.0);
    // Release and press again: it jumps again.
    step(0.0f);
    step(1.0f);
    CHECK_NEAR(engine.deck_a().transport.position_s(), 96.0, 0.5);
}

SVJ_TEST("engine: fx.2.mix moves slot 2 and no other") {
    Engine engine;
    engine.configure(120.0);
    const ControlIndex knob = engine.surface().declare("ch2.eq.hi", ControlKind::Knob);
    Mapping m;
    m.name = "knob -> fx.2.mix";
    m.source.kind = SourceKind::Control;
    m.source.control_id = "ch2.eq.hi";
    m.transform.smoothing_ms = 0.0f;
    m.destination.target = "fx.2.mix";
    engine.mapping().add(m);
    engine.bind();
    const float slot1_before = engine.rack().at(0).shared.mix;
    const float slot3_before = engine.rack().at(2).shared.mix;
    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    engine.surface().set(knob, 0.9f, 1000);
    for (int i = 0; i < 30; ++i) {
        frame.time_s += frame.dt_s;
        engine.step(frame);
    }
    CHECK_NEAR(engine.rack().at(1).shared.mix, 0.9, 0.05);
    CHECK_NEAR(engine.rack().at(0).shared.mix, slot1_before, 1e-6);
    CHECK_NEAR(engine.rack().at(2).video_override.mix, slot3_before, 1e-6);
}

SVJ_TEST("engine: mix.xfader.curve switches the curve the mixer uses") {
    Engine engine;
    engine.configure(120.0);
    const ControlIndex xf = engine.surface().declare("xfader", ControlKind::Fader);
    const ControlIndex fa = engine.surface().declare("ch1.fader", ControlKind::Fader);
    const ControlIndex fb = engine.surface().declare("ch2.fader", ControlKind::Fader);
    const ControlIndex curve = engine.surface().declare("xfader.curve", ControlKind::Button);
    Mapping m;
    m.name = "curve button -> crossfader curve";
    m.source.kind = SourceKind::Control;
    m.source.control_id = "xfader.curve";
    m.transform.smoothing_ms = 0.0f;
    m.destination.target = "mix.xfader.curve";
    engine.mapping().add(m);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    engine.surface().set(xf, 0.25f, 1);
    engine.surface().set(fa, 1.0f, 1);
    engine.surface().set(fb, 1.0f, 1);
    engine.surface().set(curve, 0.3f, 1);  // second quarter: linear
    for (int i = 0; i < 10; ++i) {
        frame.time_s += frame.dt_s;
        engine.step(frame);
    }
    CHECK(engine.mix_settings().xfader == FaderCurve::Linear);
    CHECK_NEAR(engine.weights().b, 0.25, 1e-3);  // linear at a quarter
    engine.surface().set(curve, 0.6f, 2);  // sharp: both open across the middle
    for (int i = 0; i < 10; ++i) {
        frame.time_s += frame.dt_s;
        engine.step(frame);
    }
    CHECK(engine.mix_settings().xfader == FaderCurve::Sharp);
    CHECK_NEAR(engine.weights().b, 1.0, 1e-3);
}

SVJ_TEST("engine: a load-next trigger reaches the front end as a request, once") {
    Engine engine;
    engine.configure(120.0);
    const ControlIndex pad = engine.surface().declare("pad.elite.a.8", ControlKind::Pad);
    Mapping m;
    m.name = "pad 8 -> next on B";
    m.source.kind = SourceKind::Control;
    m.source.control_id = "pad.elite.a.8";
    m.destination.target = "deck.b.load_next";
    engine.mapping().add(m);
    engine.bind();
    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    engine.surface().set(pad, 1.0f, 1);
    frame.time_s += frame.dt_s;
    engine.step(frame);
    frame.time_s += frame.dt_s;
    engine.step(frame);  // still held
    const EngineRequests asked = engine.take_requests();
    CHECK(asked.load_next_b);
    CHECK(!asked.load_next_a);
    CHECK(!engine.take_requests().any());  // taken once
}

SVJ_TEST("engine: installing a mapping file replaces the demo's rows and binds them") {
    Engine engine;
    engine.configure(120.0);
    Simulation simulation;
    simulation.configure(engine.surface());
    Mapping only;
    only.name = "only";
    only.source.kind = SourceKind::Control;
    only.source.control_id = "ch1.filter";
    only.destination.target = "overlay.opacity";
    const auto unknown = engine.install_mappings({only});
    CHECK(unknown.empty());
    CHECK_EQ(engine.mapping().size(), std::size_t{1});
}

SVJ_TEST("engine: without demo content the decks are empty and none claims to be a sphere") {
    // What a performer sees on launch. The fabricated decks -- one of them
    // 3840x1920 -- were scaffolding from before there was hardware or an
    // analysis pass, and an instrument that opens holding files you do not own
    // reads as a simulation. It also made the whole interface say "360",
    // because deck A was spherical.
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    CHECK_EQ(engine.deck_a().clip.frame_count, 0u);
    CHECK_EQ(engine.deck_b().clip.frame_count, 0u);
    CHECK(!engine.deck_a().clip.is_equirect());
    CHECK(!engine.deck_b().clip.is_equirect());
    CHECK(engine.deck_a().name.empty());
    CHECK_EQ(engine.library().size(), std::size_t{0});
    CHECK(engine.queue().empty());
    CHECK(!engine.overlay_layer().enabled);
}

SVJ_TEST("engine: without demo content the instrument itself is still set up") {
    // The rack, the modulators and the default routing are what the software
    // IS, not what the demo invented, so they must survive the split.
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    CHECK_EQ(engine.rack().size(), std::size_t{3});
    CHECK(engine.rack().at(0).enabled);
    CHECK_EQ(engine.modulators().size(), std::size_t{2});
    CHECK(engine.mapping().size() > 0);
    // The overlay keeps its policy: no platter may ever capture the layer that
    // holds a mask.
    CHECK(engine.overlay().clock.takeover() == TakeoverMode::Ignore);
    CHECK(engine.overlay().clock.mode() == ClipPlayMode::PingPong);
}

SVJ_TEST("engine: the demo content is still there when it is asked for") {
    Engine engine;
    engine.configure(124.0);  // the default, which the console demo relies on
    CHECK(engine.deck_a().clip.is_equirect());
    CHECK(engine.library().size() > 0);
    CHECK(engine.overlay_layer().enabled);
}

SVJ_TEST("engine: A CHANNEL FADER NOBODY HAS TOUCHED COUNTS AS OPEN") {
    // Found by loading a clip on a fresh install and seeing black. The ghost
    // rule is about what the interface DISPLAYS; the mixer still has to
    // compute, and reading an untouched fader as closed makes the instrument
    // silent until someone guesses which control to move. Open is wrong only
    // until the fader is first moved, and wrong in the direction where the
    // picture is there.
    Engine engine;
    engine.configure(120.0, Engine::DemoContent::No);
    engine.surface().declare("xfader", ControlKind::Fader);
    const ControlIndex fa = engine.surface().declare("ch1.fader", ControlKind::Fader);
    engine.surface().declare("ch2.fader", ControlKind::Fader);
    engine.bind();

    EngineFrame frame;
    frame.dt_s = 1.0f / 60.0f;
    frame.time_s = frame.dt_s;
    engine.step(frame);
    CHECK(engine.weights().a > 0.9f);
    CHECK(engine.weights().b > 0.9f);
    CHECK(engine.stack().a > 0.9f);

    // And the moment the real fader moves, the real value wins -- including
    // all the way down.
    engine.surface().set(fa, 0.0f, 1000);
    frame.time_s += frame.dt_s;
    engine.step(frame);
    CHECK_NEAR(engine.weights().a, 0.0, 1e-6);
    CHECK(engine.weights().b > 0.9f);
    // The control is still drawn as a ghost only until it is touched; once
    // touched it is known, which is what made the value win here.
    CHECK(engine.surface().at(fa).known);
}

// --- the deck as a player ----------------------------------------------------
//
// These drive a deck with a DEAD platter sample -- no position, no signal --
// which is what the desk looks like with no turntable plugged in. Before the
// player facade, a clip dropped on such a deck simply sat there.

namespace {

CacheHeader four_second_clip() {
    CacheHeader header;
    header.width = 640;
    header.height = 360;
    header.fps_num = 30;
    header.fps_den = 1;
    header.frame_count = 120;
    header.format = BlockFormat::BC1;
    return header;
}

DecoderSample dead_platter(double time_s) {
    DecoderSample sample;
    sample.time_s = time_s;
    return sample;
}

}  // namespace

SVJ_TEST("deck: A LOADED CLIP WITH NO PLATTER PLAYS ONCE TOLD TO") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 10.0);
    deck.advance(dead_platter(10.0));
    CHECK_NEAR(deck.played.position_s, 0.0, 1e-9);
    CHECK(!deck.playing());

    deck.play(10.0);
    CHECK(deck.playing());
    deck.advance(dead_platter(11.5));
    CHECK_NEAR(deck.played.position_s, 1.5, 1e-9);
    CHECK_NEAR(deck.played.velocity, 1.0, 1e-9);
}

SVJ_TEST("deck: a load at wall time opens at the head, not at wall-time-mod-duration") {
    // The bug this pins: the clock was rebuilt at time zero and first advanced
    // at t = 301 s, so a four-second clip opened one second in.
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 301.0);
    deck.play(301.0);
    deck.advance(dead_platter(301.0));
    CHECK_NEAR(deck.played.position_s, 0.0, 1e-9);
}

SVJ_TEST("deck: pause freezes the picture where it is, and play resumes at the same rate") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.clock.set_rate(-2.0, 0.0);
    deck.advance(dead_platter(1.0));
    const double before = deck.played.position_s;

    deck.pause(1.0);
    CHECK(!deck.playing());
    deck.advance(dead_platter(2.0));
    CHECK_NEAR(deck.played.position_s, before, 1e-9);
    CHECK_NEAR(deck.played.velocity, 0.0, 1e-9);

    deck.play(2.0);
    CHECK_NEAR(deck.clock.rate(), -2.0, 1e-9);
    deck.advance(dead_platter(2.25));
    CHECK_NEAR(deck.played.position_s, before - 0.5, 1e-9);
}

SVJ_TEST("deck: stop goes back to the head and stays there") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.advance(dead_platter(2.0));
    deck.stop(2.0);
    deck.advance(dead_platter(3.0));
    CHECK_NEAR(deck.played.position_s, 0.0, 1e-9);
    CHECK(!deck.playing());
}

SVJ_TEST("deck: pausing a platter deck leaves the platter and holds the frame") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.advance(dead_platter(1.0));
    deck.clock.set_source(DeckSource::Timecode, 1.0);
    deck.pause(1.0);
    CHECK(deck.clock.source() == DeckSource::FreeRun);
    deck.advance(dead_platter(5.0));
    CHECK_NEAR(deck.played.position_s, 1.0, 1e-9);
}

SVJ_TEST("deck: RELEASING A SCRUB RETURNS TO THE SOURCE IT WAS TAKEN FROM") {
    // The bug this pins: the clock alone forgets where the position came from,
    // so every released scrub was handed to the turntable -- which, with no
    // turntable, froze a deck that had been playing.
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.advance(dead_platter(1.0));

    deck.grab(2.0, 1.0);
    deck.advance(dead_platter(1.1));
    CHECK(deck.clock.source() == DeckSource::Hand);
    deck.scrub(2.5, 1.2);
    deck.advance(dead_platter(1.2));
    CHECK_NEAR(deck.played.position_s, 2.5, 1e-9);

    deck.release(1.2);
    CHECK(deck.clock.source() == DeckSource::FreeRun);
    CHECK(deck.playing());
    deck.advance(dead_platter(1.7));
    CHECK_NEAR(deck.played.position_s, 3.0, 1e-9);
}

SVJ_TEST("deck: a scrub taken from the platter goes back to the platter") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.clock.set_source(DeckSource::Timecode, 0.0);
    deck.grab(1.0, 0.0);
    deck.release(0.5);
    CHECK(deck.clock.source() == DeckSource::Timecode);
}

SVJ_TEST("deck: reloading the same clip with keep_position holds the frame") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.advance(dead_platter(2.5));
    CacheHeader sphere = four_second_clip();
    sphere.flags = kCacheEquirect;
    deck.load(sphere, "rush", 124.0, 2.5, true);
    CHECK(deck.clip.is_equirect());
    CHECK(deck.playing());
    deck.advance(dead_platter(2.5));
    CHECK_NEAR(deck.played.position_s, 2.5, 1e-9);
}

// --- the pads' commands --------------------------------------------------------------

SVJ_TEST("engine: A CUE SET FROM THE FRONT END LANDS WHERE THE DECK IS, AND A PAD JUMPS TO IT") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.transport.set_quantise(false);  // "where the deck is", not "on the beat"

    EngineFrame frame;
    frame.deck_a = dead_platter(1.5);
    frame.deck_b = dead_platter(1.5);
    frame.time_s = 1.5;
    engine.step(frame);  // the deck is at 1.5 s now

    frame.time_s = 1.5;
    frame.commands_a.cue_set = true;
    frame.commands_a.cue_index = 4;
    engine.step(frame);
    CHECK(deck.transport.cue(4).set);
    CHECK_NEAR(deck.transport.cue(4).position_s, 1.5, 1e-6);

    frame.commands_a = DeckCommands{};
    frame.time_s = 3.0;
    frame.deck_a = dead_platter(3.0);
    engine.step(frame);
    CHECK_NEAR(deck.played.position_s, 3.0, 1e-6);

    frame.commands_a.cue_jump = true;
    frame.commands_a.cue_index = 4;
    frame.deck_a = dead_platter(3.0);
    engine.step(frame);
    CHECK_NEAR(deck.played.position_s, 1.5, 1e-6);

    frame.commands_a = DeckCommands{};
    frame.commands_a.cue_clear = true;
    frame.commands_a.cue_index = 4;
    engine.step(frame);
    CHECK(!deck.transport.cue(4).set);
}

SVJ_TEST("engine: setting and jumping on the same pad press sets and does not jump") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.transport.set_quantise(false);
    deck.transport.set_cue(2, 0.5);

    EngineFrame frame;
    frame.deck_a = dead_platter(2.0);
    frame.deck_b = dead_platter(2.0);
    frame.time_s = 2.0;
    engine.step(frame);
    frame.commands_a.cue_set = true;
    frame.commands_a.cue_jump = true;
    frame.commands_a.cue_index = 2;
    engine.step(frame);
    CHECK_NEAR(deck.transport.cue(2).position_s, 2.0, 1e-6);
    CHECK_NEAR(deck.played.position_s, 2.0, 1e-6);
}

SVJ_TEST("engine: an auto loop of four beats closes four beats after the position") {
    Engine engine;
    engine.configure(120.0, Engine::DemoContent::No);  // one beat = 0.5 s
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 120.0, 0.0);
    deck.play(0.0);
    deck.transport.set_quantise(false);

    EngineFrame frame;
    frame.deck_a = dead_platter(1.0);
    frame.deck_b = dead_platter(1.0);
    frame.time_s = 1.0;
    engine.step(frame);
    frame.commands_a.auto_loop_beats = 4.0;
    engine.step(frame);
    CHECK(deck.transport.loop().active);
    CHECK_NEAR(deck.transport.loop().length_s(), 2.0, 1e-6);
}

SVJ_TEST("engine: a beat jump moves the deck by that many beats, backwards included") {
    Engine engine;
    engine.configure(120.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 120.0, 0.0);
    deck.play(0.0);
    deck.transport.set_quantise(false);

    EngineFrame frame;
    frame.deck_a = dead_platter(2.0);
    frame.deck_b = dead_platter(2.0);
    frame.time_s = 2.0;
    engine.step(frame);
    frame.commands_a.beat_jump_beats = -1.0;
    engine.step(frame);
    CHECK_NEAR(deck.played.position_s, 1.5, 1e-6);
}

SVJ_TEST("engine: anchoring now arms the anchor at deck A's positions") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    deck.load(four_second_clip(), "rush", 124.0, 0.0);
    deck.play(0.0);
    deck.advance(dead_platter(1.25));
    engine.anchor_now(1.25);
    CHECK(engine.anchor().armed());
    CHECK_NEAR(engine.anchor().clip_position(deck.timecode.state().position_s), 1.25, 1e-6);
}

SVJ_TEST("engine: the VRAM budget reaches a loaded deck and every later load") {
    Engine engine;
    engine.configure(124.0, Engine::DemoContent::No);
    Deck& deck = engine.deck_a();
    CacheHeader header = four_second_clip();
    header.frame_count = 100000;  // more than any budget holds
    deck.load(header, "long", 124.0, 0.0);
    const std::uint32_t before = deck.window.capacity();
    engine.set_window_budget(64ull << 20);
    CHECK(deck.window.capacity() < before);
    CHECK_EQ(deck.window.capacity(),
             static_cast<std::uint32_t>((64ull << 20) /
                                        block_bytes_per_frame(640, 360, BlockFormat::BC1)));
    engine.deck_b().load(header, "long", 124.0, 0.0);
    CHECK_EQ(engine.deck_b().window.capacity(), deck.window.capacity());
}

SVJ_TEST("engine: no default mapping names a destination nothing transmits") {
    // The OSC rows went to an address no transport carried; a list that shows
    // a mapping doing nothing teaches a performer to distrust the others.
    Engine engine;
    engine.configure(124.0);
    for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
        CHECK(engine.mapping().at(i).destination.kind == DestinationKind::Local);
        CHECK(engine.mapping().at(i).destination.target != "mix.transition");
    }
}
