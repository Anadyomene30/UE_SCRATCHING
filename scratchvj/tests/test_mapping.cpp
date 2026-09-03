#include "core/mapping.h"
#include "core/protocol.h"
#include "harness.h"

using namespace svj;

namespace {

Mapping control_mapping(std::string control, std::string target) {
    Mapping m;
    m.name = control + " -> " + target;
    m.source.kind = SourceKind::Control;
    m.source.control_id = std::move(control);
    m.destination.kind = DestinationKind::Local;
    m.destination.target = std::move(target);
    return m;
}

}  // namespace

SVJ_TEST("mapping: a control drives its destination through the transform") {
    Surface surface;
    const ControlIndex knob = surface.declare("ch1.eq.hi", ControlKind::Knob);

    MappingEngine engine;
    Mapping m = control_mapping("ch1.eq.hi", "deck.a.yaw");
    m.transform.out_lo = -180.0f;
    m.transform.out_hi = 180.0f;
    const std::size_t index = engine.add(m);

    CHECK(engine.resolve(surface).empty());

    surface.set(knob, 1.0f, 1);
    engine.evaluate(surface, EngineInputs{}, 0.016f);
    CHECK(engine.active(index));
    CHECK_NEAR(engine.value(index), 180.0, 1e-4);

    surface.set(knob, 0.5f, 2);
    engine.evaluate(surface, EngineInputs{}, 0.016f);
    CHECK_NEAR(engine.value(index), 0.0, 1e-4);
}

SVJ_TEST("mapping: one control can feed several destinations with different curves") {
    Surface surface;
    const ControlIndex filter = surface.declare("ch1.filter", ControlKind::Knob);

    MappingEngine engine;
    Mapping cutoff = control_mapping("ch1.filter", "fx.a.filter.cutoff");
    cutoff.transform.curve = CurveKind::Logarithmic;
    cutoff.transform.shape = 2.0f;

    Mapping blur = control_mapping("ch1.filter", "fx.a.filter.blur");
    blur.transform.out_lo = 0.0f;
    blur.transform.out_hi = 20.0f;

    const std::size_t a = engine.add(cutoff);
    const std::size_t b = engine.add(blur);
    engine.resolve(surface);

    surface.set(filter, 0.25f, 1);
    engine.evaluate(surface, EngineInputs{}, 0.016f);
    CHECK_NEAR(engine.value(a), 0.5, 1e-4);
    CHECK_NEAR(engine.value(b), 5.0, 1e-4);
}

SVJ_TEST("mapping: an unresolved control is reported and left inactive") {
    Surface surface;
    surface.declare("xfader", ControlKind::Fader);

    MappingEngine engine;
    const std::size_t stale = engine.add(control_mapping("ch9.ghost", "fx.a.echo.mix"));

    const auto unresolved = engine.resolve(surface);
    CHECK_EQ(unresolved.size(), std::size_t{1});
    CHECK_EQ(unresolved[0], std::string("ch9.ghost"));

    // Critically it must stay inactive rather than evaluate to zero: a stale
    // mapping file must not slam a parameter to the bottom of its range.
    engine.evaluate(surface, EngineInputs{}, 0.016f);
    CHECK(!engine.active(stale));
}

SVJ_TEST("mapping: a disabled row is skipped") {
    Surface surface;
    const ControlIndex knob = surface.declare("ch1.low", ControlKind::Knob);
    MappingEngine engine;
    Mapping m = control_mapping("ch1.low", "fx.a.echo.mix");
    m.enabled = false;
    const std::size_t index = engine.add(m);
    engine.resolve(surface);

    surface.set(knob, 1.0f, 1);
    engine.evaluate(surface, EngineInputs{}, 0.016f);
    CHECK(!engine.active(index));
}

SVJ_TEST("mapping: deck-derived sources need no surface control") {
    MappingEngine engine;
    Mapping m;
    m.name = "scratch rate to shake";
    m.source.kind = SourceKind::DeckScratchRate;
    m.source.deck = 0;
    m.transform.in_hi = 12.0f;
    m.destination.kind = DestinationKind::Osc;
    m.destination.target = "/ue/shake";
    const std::size_t index = engine.add(m);

    Surface surface;
    CHECK(engine.resolve(surface).empty());  // nothing to resolve, so nothing missing

    EngineInputs inputs;
    inputs.deck[0].scratch_rate = 6.0f;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK(engine.active(index));
    CHECK_NEAR(engine.value(index), 0.5, 1e-5);
}

SVJ_TEST("mapping: deck selection picks the right deck") {
    MappingEngine engine;
    Mapping m;
    m.source.kind = SourceKind::DeckVelocity;
    m.source.deck = 1;
    m.transform.in_lo = -4.0f;
    m.transform.in_hi = 4.0f;
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);

    EngineInputs inputs;
    inputs.deck[0].velocity = -4.0f;
    inputs.deck[1].velocity = 4.0f;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 1.0, 1e-5);
}

SVJ_TEST("mapping: an out-of-range deck index is clamped rather than read out of bounds") {
    MappingEngine engine;
    Mapping m;
    m.source.kind = SourceKind::DeckVelocity;
    m.source.deck = 200;
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);
    EngineInputs inputs;
    inputs.deck[1].velocity = 1.0f;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 1.0, 1e-5);
}

SVJ_TEST("mapping: a gesture source reads as a switch") {
    MappingEngine engine;
    Mapping m;
    m.source.kind = SourceKind::Gesture;
    m.source.gesture_bit = kGestureBackspinA;
    m.destination.kind = DestinationKind::Osc;
    m.destination.target = "/ue/whippan";
    const std::size_t index = engine.add(m);

    Surface surface;
    engine.resolve(surface);

    EngineInputs inputs;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 0.0, 1e-6);

    inputs.gesture_bits = kGestureBackspinA | kGestureScratchingB;
    engine.evaluate(surface, inputs, 0.016f);
    CHECK_NEAR(engine.value(index), 1.0, 1e-6);
}

SVJ_TEST("mapping: smoothing carries across evaluations") {
    Surface surface;
    const ControlIndex knob = surface.declare("ch1.mid", ControlKind::Knob);
    MappingEngine engine;
    Mapping m = control_mapping("ch1.mid", "fx.a.echo.mix");
    m.transform.smoothing_ms = 100.0f;
    const std::size_t index = engine.add(m);
    engine.resolve(surface);

    surface.set(knob, 0.0f, 1);
    engine.evaluate(surface, EngineInputs{}, 0.01f);
    CHECK_NEAR(engine.value(index), 0.0, 1e-6);

    surface.set(knob, 1.0f, 2);
    engine.evaluate(surface, EngineInputs{}, 0.01f);
    const float first = engine.value(index);
    CHECK(first > 0.0f);
    CHECK(first < 1.0f);  // still on its way

    for (int i = 0; i < 200; ++i) engine.evaluate(surface, EngineInputs{}, 0.01f);
    CHECK_NEAR(engine.value(index), 1.0, 1e-3);
}

SVJ_TEST("mapping: clearing empties the engine") {
    MappingEngine engine;
    engine.add(control_mapping("a", "b"));
    engine.clear();
    CHECK_EQ(engine.size(), std::size_t{0});
}
