#include <cmath>

#include "core/mapping.h"
#include "core/modulator.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("lfo: a free-running sine sweeps its whole range over one period") {
    Lfo lfo;
    lfo.shape = LfoShape::Sine;
    lfo.rate_hz = 1.0;

    CHECK_NEAR(lfo_value(lfo, 0.00, 0.0), 0.0, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 0.25, 0.0), 0.5, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 0.50, 0.0), 1.0, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 1.00, 0.0), 0.0, 1e-6);
}

SVJ_TEST("lfo: every shape stays inside its range") {
    for (const LfoShape shape : {LfoShape::Sine, LfoShape::Triangle, LfoShape::RampUp,
                                 LfoShape::RampDown, LfoShape::Square}) {
        Lfo lfo;
        lfo.shape = shape;
        for (int i = 0; i < 200; ++i) {
            const float v = lfo_value(lfo, i * 0.01, 0.0);
            CHECK(v >= -1e-6f);
            CHECK(v <= 1.0f + 1e-6f);
        }
    }
}

SVJ_TEST("lfo: bipolar swings around zero instead of around the centre") {
    Lfo lfo;
    lfo.shape = LfoShape::Triangle;
    lfo.bipolar = true;
    lfo.rate_hz = 1.0;
    CHECK_NEAR(lfo_value(lfo, 0.0, 0.0), -1.0, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 0.5, 0.0), 1.0, 1e-6);
}

SVJ_TEST("lfo: depth and centre place the output where asked") {
    Lfo lfo;
    lfo.shape = LfoShape::RampUp;
    lfo.rate_hz = 1.0;
    lfo.depth = 0.4f;
    lfo.centre = 0.5f;
    CHECK_NEAR(lfo_value(lfo, 0.0, 0.0), 0.3, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 0.999999, 0.0), 0.7, 1e-5);
}

SVJ_TEST("lfo: a tempo-synced LFO reads its phase from the beat position") {
    Lfo lfo;
    lfo.shape = LfoShape::RampUp;
    lfo.beats = 4.0;
    // Time is ignored entirely once beats is set.
    CHECK_NEAR(lfo_value(lfo, 999.0, 0.0), 0.0, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 999.0, 2.0), 0.5, 1e-6);
    CHECK_NEAR(lfo_value(lfo, 999.0, 4.0), 0.0, 1e-6);  // wrapped
    CHECK_NEAR(lfo_value(lfo, 999.0, 6.0), 0.5, 1e-6);
}

SVJ_TEST("lfo: A TEMPO-SYNCED LFO FOLLOWS THE PLATTER BACKWARDS") {
    // Deriving the phase rather than counting it is what makes this work. A
    // counted phase would keep marching forward while the record went back, and
    // the whole thing would stop being scratchable.
    Lfo lfo;
    lfo.shape = LfoShape::Triangle;
    lfo.beats = 2.0;

    const float forward = lfo_value(lfo, 0.0, 3.0);
    const float backward = lfo_value(lfo, 0.0, 3.0 - 2.0);  // one period earlier
    CHECK_NEAR(forward, backward, 1e-6);

    // And a negative beat position is meaningful rather than clamped.
    CHECK_NEAR(lfo_value(lfo, 0.0, -0.5), lfo_value(lfo, 0.0, 1.5), 1e-6);
}

SVJ_TEST("lfo: the phase offset shifts the waveform") {
    Lfo a, b;
    a.shape = LfoShape::RampUp;
    a.beats = 4.0;
    b = a;
    b.phase_offset = 0.25;
    CHECK_NEAR(lfo_value(b, 0.0, 0.0), 0.25, 1e-6);
    CHECK_NEAR(lfo_value(b, 0.0, 3.0), lfo_value(a, 0.0, 4.0), 1e-6);
}

SVJ_TEST("envelope: it rises fast and falls slowly") {
    Envelope settings;
    settings.attack_ms = 5.0f;
    settings.release_ms = 200.0f;
    EnvelopeFollower follower(settings);

    for (int i = 0; i < 20; ++i) follower.update(1.0f, 0.001f);
    const float risen = follower.value();
    CHECK(risen > 0.9f);

    for (int i = 0; i < 20; ++i) follower.update(0.0f, 0.001f);
    CHECK(follower.value() > 0.8f);  // still holding on, which is the point
}

SVJ_TEST("envelope: it settles on a steady input") {
    EnvelopeFollower follower;
    for (int i = 0; i < 2000; ++i) follower.update(0.6f, 0.001f);
    CHECK_NEAR(follower.value(), 0.6, 1e-3);
}

SVJ_TEST("envelope: zero time constants pass the input straight through") {
    Envelope settings;
    settings.attack_ms = 0.0f;
    settings.release_ms = 0.0f;
    EnvelopeFollower follower(settings);
    CHECK_NEAR(follower.update(0.75f, 0.01f), 0.75, 1e-6);
}

SVJ_TEST("bank: LFOs and envelopes come out as one flat array") {
    ModulatorBank bank;
    Lfo lfo;
    lfo.shape = LfoShape::RampUp;
    lfo.beats = 4.0;
    const std::size_t lfo_index = bank.add(lfo);

    Envelope envelope;
    envelope.attack_ms = 1.0f;
    const std::size_t env_index = bank.add(envelope);

    CHECK_EQ(bank.size(), std::size_t{2});
    bank.set_envelope_input(env_index, 1.0f);
    for (int i = 0; i < 50; ++i) bank.update(0.0, 2.0, 0.001f);

    CHECK_NEAR(bank.values()[lfo_index], 0.5, 1e-6);
    CHECK(bank.values()[env_index] > 0.9f);
}

SVJ_TEST("bank: setting the input of a slot that is not an envelope is harmless") {
    ModulatorBank bank;
    const std::size_t lfo_index = bank.add(Lfo{});
    bank.set_envelope_input(lfo_index, 1.0f);
    bank.set_envelope_input(99, 1.0f);
    bank.update(0.0, 0.0, 0.01f);
    CHECK_EQ(bank.size(), std::size_t{1});
}

SVJ_TEST("mapping: a modulator drives a destination like any other source") {
    ModulatorBank bank;
    Lfo lfo;
    lfo.shape = LfoShape::RampUp;
    lfo.beats = 4.0;
    bank.add(lfo);
    bank.update(0.0, 1.0, 0.01f);  // quarter of the way through

    MappingEngine engine;
    Mapping m;
    m.name = "LFO 1 -> rotation du kaléidoscope";
    m.source.kind = SourceKind::Modulator;
    m.source.index = 0;
    m.transform.out_lo = 0.0f;
    m.transform.out_hi = 360.0f;
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);

    EngineInputs inputs;
    inputs.modulators = bank.values().data();
    inputs.modulator_count = bank.size();
    engine.evaluate(surface, inputs, 0.016f);

    CHECK(engine.active(index));
    CHECK_NEAR(engine.value(index), 90.0, 1e-3);
}

SVJ_TEST("mapping: an audio band drives a destination") {
    const float bands[3] = {0.8f, 0.2f, 0.05f};

    MappingEngine engine;
    Mapping m;
    m.name = "graves -> bloom";
    m.source.kind = SourceKind::AudioBand;
    m.source.index = 0;
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);

    EngineInputs inputs;
    inputs.bands = bands;
    inputs.band_count = 3;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 0.8, 1e-6);
}

SVJ_TEST("mapping: a modulator index with nothing behind it reads as zero, not garbage") {
    MappingEngine engine;
    Mapping m;
    m.source.kind = SourceKind::Modulator;
    m.source.index = 7;
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);
    EngineInputs inputs;  // no arrays supplied at all
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 0.0, 1e-6);
}
