#include "core/learn.h"
#include "harness.h"

using namespace svj;

namespace {

MidiEvent cc(std::uint8_t number, std::uint16_t value, std::uint8_t channel = 0) {
    MidiEvent e;
    e.kind = MidiKind::ControlChange;
    e.channel = channel;
    e.number = number;
    e.value = value;
    return e;
}

MidiEvent note(std::uint8_t number, std::uint16_t velocity, std::uint8_t channel = 0) {
    MidiEvent e;
    e.kind = velocity == 0 ? MidiKind::NoteOff : MidiKind::NoteOn;
    e.channel = channel;
    e.number = number;
    e.value = velocity;
    return e;
}

void sweep(MidiLearn& learn, std::uint8_t number, int steps) {
    for (int i = 0; i < steps; ++i) learn.observe(cc(number, static_cast<std::uint16_t>(i * 7)));
}

}  // namespace

SVJ_TEST("learn: a swept knob binds after enough distinct values") {
    MidiLearn learn(6);
    learn.begin({LearnTarget{"ch1.filter", ControlKind::Knob}});
    CHECK(learn.active());

    sweep(learn, 12, 5);
    CHECK(learn.active());  // not yet convinced

    learn.observe(cc(12, 99));
    CHECK(!learn.active());
    CHECK_EQ(learn.results().size(), std::size_t{1});
    CHECK_EQ(learn.results()[0].id, std::string("ch1.filter"));
    CHECK_EQ(int(learn.results()[0].address.number), 12);
}

SVJ_TEST("learn: a stray single event does not steal the binding") {
    // This is the whole reason for the distinct-value threshold: a neighbouring
    // knob brushed in passing must not win.
    MidiLearn learn(6);
    learn.begin({LearnTarget{"ch1.filter", ControlKind::Knob}});

    learn.observe(cc(99, 64));  // brushed by accident
    sweep(learn, 12, 8);

    CHECK_EQ(learn.results().size(), std::size_t{1});
    CHECK_EQ(int(learn.results()[0].address.number), 12);
}

SVJ_TEST("learn: repeating the same value does not count as movement") {
    MidiLearn learn(4);
    learn.begin({LearnTarget{"ch1.filter", ControlKind::Knob}});
    for (int i = 0; i < 20; ++i) learn.observe(cc(12, 64));
    CHECK(learn.active());
    CHECK(learn.results().empty());
}

SVJ_TEST("learn: a pad binds on a single press") {
    MidiLearn learn(6);
    learn.begin({LearnTarget{"pad.elite.a.1", ControlKind::Pad}});
    CHECK(learn.observe(note(40, 127)));
    CHECK(!learn.active());
    CHECK(learn.results()[0].address.kind == MidiKind::NoteOn);
    CHECK_EQ(int(learn.results()[0].address.number), 40);
}

SVJ_TEST("learn: a pad release alone binds nothing") {
    MidiLearn learn(6);
    learn.begin({LearnTarget{"pad.elite.a.1", ControlKind::Pad}});
    CHECK(!learn.observe(note(40, 0)));
    CHECK(learn.active());
}

SVJ_TEST("learn: an address already claimed cannot be bound twice") {
    // A knob still settling must not steal the control learned before it.
    MidiLearn learn(4);
    learn.begin({LearnTarget{"first", ControlKind::Knob}, LearnTarget{"second", ControlKind::Knob}});

    sweep(learn, 12, 6);
    CHECK_EQ(learn.results().size(), std::size_t{1});

    sweep(learn, 12, 10);  // the same knob, still moving
    CHECK(learn.active());
    CHECK_EQ(learn.results().size(), std::size_t{1});

    sweep(learn, 13, 6);
    CHECK_EQ(learn.results().size(), std::size_t{2});
    CHECK_EQ(int(learn.results()[1].address.number), 13);
}

SVJ_TEST("learn: targets are walked in order and skipping leaves one unbound") {
    MidiLearn learn(4);
    learn.begin({LearnTarget{"a", ControlKind::Knob}, LearnTarget{"b", ControlKind::Knob},
                 LearnTarget{"c", ControlKind::Knob}});
    CHECK_EQ(learn.current().id, std::string("a"));
    CHECK_EQ(learn.remaining(), std::size_t{3});

    learn.skip();
    CHECK_EQ(learn.current().id, std::string("b"));
    sweep(learn, 20, 6);
    CHECK_EQ(learn.current().id, std::string("c"));

    CHECK_EQ(learn.results().size(), std::size_t{1});
    CHECK_EQ(learn.results()[0].id, std::string("b"));
}

SVJ_TEST("learn: the same CC number on another channel is a different address") {
    MidiLearn learn(4);
    learn.begin({LearnTarget{"a", ControlKind::Knob}, LearnTarget{"b", ControlKind::Knob}});
    for (int i = 0; i < 6; ++i) learn.observe(cc(12, static_cast<std::uint16_t>(i * 3), 0));
    for (int i = 0; i < 6; ++i) learn.observe(cc(12, static_cast<std::uint16_t>(i * 3), 1));
    CHECK_EQ(learn.results().size(), std::size_t{2});
    CHECK_EQ(int(learn.results()[1].address.channel), 1);
}

SVJ_TEST("learn: applying results declares and binds every control") {
    MidiLearn learn(4);
    learn.begin({LearnTarget{"xfader", ControlKind::Fader},
                 LearnTarget{"pad.elite.a.1", ControlKind::Pad}});
    sweep(learn, 7, 6);
    learn.observe(note(40, 127));

    Surface surface;
    learn.apply_to(surface);
    CHECK_EQ(surface.size(), std::size_t{2});

    const ControlIndex xfader = surface.find("xfader");
    CHECK(xfader != kNoControl);
    CHECK_EQ(surface.apply(cc(7, 127), 1), xfader);
    CHECK_NEAR(surface.at(xfader).value, 1.0, 1e-6);

    const ControlIndex pad = surface.find("pad.elite.a.1");
    CHECK_EQ(surface.apply(note(40, 127), 2), pad);
}

SVJ_TEST("learn: observing after the session ends is harmless") {
    MidiLearn learn(2);
    learn.begin({LearnTarget{"a", ControlKind::Knob}});
    sweep(learn, 5, 4);
    CHECK(!learn.active());
    CHECK(!learn.observe(cc(6, 10)));
    CHECK_EQ(learn.results().size(), std::size_t{1});
}
