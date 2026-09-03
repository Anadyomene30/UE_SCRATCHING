#include "core/surface.h"
#include "harness.h"

using namespace svj;

namespace {

MidiEvent cc(std::uint8_t channel, std::uint8_t number, std::uint16_t value) {
    MidiEvent e;
    e.kind = MidiKind::ControlChange;
    e.channel = channel;
    e.number = number;
    e.value = value;
    return e;
}

}  // namespace

SVJ_TEST("surface: declaring is idempotent and preserves the existing value") {
    Surface surface;
    const ControlIndex first = surface.declare("ch1.eq.hi", ControlKind::Knob);
    surface.set(first, 0.7f, 100);
    const ControlIndex again = surface.declare("ch1.eq.hi", ControlKind::Knob);
    CHECK_EQ(first, again);
    CHECK_EQ(surface.size(), std::size_t{1});
    CHECK_NEAR(surface.at(first).value, 0.7, 1e-6);
}

SVJ_TEST("surface: an unknown id resolves to no control") {
    Surface surface;
    surface.declare("xfader", ControlKind::Fader);
    CHECK_EQ(surface.find("nope"), kNoControl);
}

SVJ_TEST("surface: controls start unknown and become known once touched") {
    Surface surface;
    const ControlIndex knob = surface.declare("ch1.filter", ControlKind::Knob);
    const ControlIndex fader = surface.declare("ch1.fader", ControlKind::Fader);
    CHECK(!surface.at(knob).known);
    CHECK_EQ(surface.unknown_count(), std::size_t{2});

    surface.set(knob, 0.5f, 42);
    CHECK(surface.at(knob).known);
    CHECK_EQ(surface.unknown_count(), std::size_t{1});
    CHECK(!surface.at(fader).known);
}

SVJ_TEST("surface: a bound MIDI event moves its control") {
    Surface surface;
    const ControlIndex xfader = surface.declare("xfader", ControlKind::Fader);
    MidiAddress address;
    address.kind = MidiKind::ControlChange;
    address.channel = 0;
    address.number = 7;
    surface.bind(address, xfader);

    const ControlIndex moved = surface.apply(cc(0, 7, 127), 1000);
    CHECK_EQ(moved, xfader);
    CHECK_NEAR(surface.at(xfader).value, 1.0, 1e-6);
    CHECK_EQ(surface.at(xfader).last_touch_us, std::uint64_t{1000});
    CHECK_EQ(surface.last_touched(), xfader);
}

SVJ_TEST("surface: an unbound event moves nothing") {
    Surface surface;
    surface.declare("xfader", ControlKind::Fader);
    CHECK_EQ(surface.apply(cc(0, 99, 64), 1), kNoControl);
    CHECK_EQ(surface.unknown_count(), std::size_t{1});
}

SVJ_TEST("surface: note on and note off address the same pad") {
    Surface surface;
    const ControlIndex pad = surface.declare("pad.elite.a.1", ControlKind::Pad);

    MidiEvent press;
    press.kind = MidiKind::NoteOn;
    press.channel = 2;
    press.number = 40;
    press.value = 127;
    surface.bind(MidiAddress::from(press), pad);

    CHECK_EQ(surface.apply(press, 10), pad);
    CHECK_NEAR(surface.at(pad).value, 1.0, 1e-6);

    MidiEvent release = press;
    release.kind = MidiKind::NoteOff;
    release.value = 0;
    CHECK_EQ(surface.apply(release, 20), pad);
    CHECK_NEAR(surface.at(pad).value, 0.0, 1e-6);
}

SVJ_TEST("surface: rebinding an address moves it to the new control") {
    Surface surface;
    const ControlIndex a = surface.declare("a", ControlKind::Knob);
    const ControlIndex b = surface.declare("b", ControlKind::Knob);
    MidiAddress address;
    address.number = 5;

    surface.bind(address, a);
    surface.bind(address, b);
    CHECK_EQ(surface.bound_to(address), b);
    CHECK_EQ(surface.apply(cc(0, 5, 64), 1), b);
}

SVJ_TEST("surface: forgetting positions keeps values and bindings but clears known") {
    Surface surface;
    const ControlIndex knob = surface.declare("ch1.low", ControlKind::Knob);
    MidiAddress address;
    address.number = 3;
    surface.bind(address, knob);
    surface.set(knob, 0.3f, 5);

    surface.forget_positions();
    CHECK(!surface.at(knob).known);
    CHECK_NEAR(surface.at(knob).value, 0.3, 1e-6);  // last seen value is still shown
    CHECK_EQ(surface.bound_to(address), knob);
    CHECK_EQ(surface.last_touched(), kNoControl);
}

SVJ_TEST("surface: a 14-bit control change gives finer resolution than 7-bit") {
    Surface surface;
    const ControlIndex fader = surface.declare("ch1.fader", ControlKind::Fader);
    MidiAddress address;
    address.number = 1;
    surface.bind(address, fader);

    MidiEvent fine;
    fine.kind = MidiKind::ControlChange;
    fine.number = 1;
    fine.value = 8192;
    fine.high_resolution = true;
    surface.apply(fine, 1);
    CHECK_NEAR(surface.at(fader).value, 8192.0 / 16383.0, 1e-6);
}

SVJ_TEST("surface: out-of-range indices are rejected") {
    Surface surface;
    surface.set(kNoControl, 0.5f, 1);  // must not crash
    surface.set(99, 0.5f, 1);
    bool threw = false;
    try {
        surface.at(99);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}
