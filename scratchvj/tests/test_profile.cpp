#include <cstdio>
#include <algorithm>
#include <set>
#include <string>

#include "core/layout.h"
#include "core/profile.h"
#include "harness.h"

using namespace svj;

namespace {

bool has(const DeviceProfile& p, const std::string& id) {
    return profile_control(p, id) != nullptr;
}

}  // namespace

SVJ_TEST("profile: the Elite's roster is the two-channel mixer the manual describes") {
    // Two strips, not four: the Elite is a battle mixer. Two FX units, a loop
    // section per side, sixteen pads with four modes per side, browse, the
    // outputs and the front panel.
    const DeviceProfile& elite = builtin_elite();
    CHECK(has(elite, "ch1.trim"));
    CHECK(has(elite, "ch2.fader"));
    CHECK(!has(elite, "ch3.fader"));
    CHECK(has(elite, "fx.a.level"));
    CHECK(has(elite, "fx.b.beats"));
    CHECK(has(elite, "loop.a.encoder"));
    CHECK(has(elite, "pad.elite.b.8"));
    CHECK(has(elite, "pad.elite.a.mode.4"));
    CHECK(has(elite, "browse.encoder"));
    CHECK(has(elite, "master.level"));
    CHECK(has(elite, "xfader.curve"));
    CHECK(elite.controls.size() >= 80);

    // Measured on Reloop's own render, and each of these was guessed wrong
    // before it was: there are no channel cue buttons (monitoring is a slider
    // in the centre), one SHIFT rather than one a side, and three curve and
    // reverse pairs on the front edge rather than two.
    CHECK(!has(elite, "ch1.cue"));
    CHECK(!has(elite, "ch2.cue"));
    CHECK(has(elite, "cue.select"));
    CHECK(has(elite, "shift"));
    CHECK(!has(elite, "shift.a"));
    CHECK(has(elite, "fader.a.curve"));
    CHECK(has(elite, "fader.b.curve"));
    CHECK(has(elite, "fx.a.hold"));
    CHECK(has(elite, "fx.a.sampler"));
    CHECK(has(elite, "loop.a.inout"));
}

SVJ_TEST("profile: the Elite carries the panel it was measured on") {
    // The whole point of the geometry: a surface drawn where the hardware has
    // it is a mirror of what is under the hand, and one drawn as a list is a
    // second panel to learn. 290 x 400 mm, portrait, like every battle mixer.
    const DeviceProfile& elite = builtin_elite();
    CHECK(profile_has_geometry(elite));
    CHECK_NEAR(elite.panel_w, 290.0, 1e-3);
    CHECK_NEAR(elite.panel_h, 400.0, 1e-3);
    CHECK(elite.panel_w < elite.panel_h);

    // The crossfader is at the front, the FX units at the rear corners, and
    // the pads in the band between them.
    const auto find = [&elite](const std::string& title) -> const ProfileGroup* {
        for (const ProfileGroup& g : elite.layout) {
            if (g.title == title) return &g;
        }
        return nullptr;
    };
    const ProfileGroup* xf = find("CROSSFADER");
    const ProfileGroup* fx1 = find("FX 1");
    const ProfileGroup* fx2 = find("FX 2");
    const ProfileGroup* pads1 = find("PADS 1");
    CHECK(xf != nullptr && fx1 != nullptr && fx2 != nullptr && pads1 != nullptr);
    CHECK(fx1->y < pads1->y);
    CHECK(pads1->y < xf->y);
    CHECK(fx1->x < fx2->x);
    // Mirror symmetry about the centre line, to within a millimetre.
    CHECK_NEAR(fx1->x + fx1->w / 2.0 + fx2->x + fx2->w / 2.0, elite.panel_w, 1.0);
    // And the front edge is not on the top panel.
    for (const ProfileGroup& g : elite.layout) {
        if (g.title == "COURBES") CHECK(g.front_panel);
    }
}

SVJ_TEST("profile: a turntable with no measured panel is drawn as a list, not a guess") {
    // Not a gap waiting to be filled but a decision, and the roadmap says why:
    // the panel model gives one identifier one place, and a layer is state
    // inside the turntable that nothing here can see.
    CHECK(!profile_has_geometry(rp8000_profile('a')));
}

SVJ_TEST("profile: the turntable says its three layers are the same eight pads") {
    // Twenty-four identifiers for EIGHT physical pads, drawn as three groups
    // of eight side by side. Complete, and read as twenty-four buttons by the
    // person who then planned a panel for them. A list can be right and still
    // mislead, so the device carries the sentence that the list cannot.
    const DeviceProfile p = rp8000_profile('a');

    int pads = 0;
    for (const ProfileControl& c : p.controls) {
        if (c.kind == ControlKind::Pad) ++pads;
    }
    CHECK_EQ(pads, 24);
    CHECK(!p.note.empty());
}

SVJ_TEST("profile: the Elite's fader-curve controls are optional, because their MIDI is unmeasured") {
    // Learn must pass over them without leaving the session unfinished; a
    // roster that demanded a sweep of a button that may be silent would never
    // complete on the real desk.
    const DeviceProfile& elite = builtin_elite();
    for (const char* id : {"xfader.curve", "xfader.reverse", "fader.a.curve", "fader.b.reverse"}) {
        const ProfileControl* c = profile_control(elite, id);
        CHECK(c != nullptr);
        CHECK(c->optional);
    }
    CHECK(!profile_control(elite, "xfader")->optional);
}

SVJ_TEST("profile: the Elite's encoders are relative, its pots absolute") {
    const DeviceProfile& elite = builtin_elite();
    CHECK(profile_control(elite, "browse.encoder")->encoder == EncoderMode::Relative64);
    CHECK(profile_control(elite, "fx.a.beats")->kind == ControlKind::Encoder);
    CHECK(profile_control(elite, "ch1.eq.hi")->encoder == EncoderMode::Absolute);
}

SVJ_TEST("profile: every built-in profile validates and draws only what it declares") {
    for (const DeviceProfile* p : builtin_profiles()) {
        const auto faults = validate_profile(*p);
        CHECK(faults.empty());
    }
    for (const char deck : {'a', 'b'}) CHECK(validate_profile(rp8000_profile(deck)).empty());
}

SVJ_TEST("profile: a layout naming an undeclared control is a fault, not a silent gap") {
    DeviceProfile p;
    p.name = "x";
    p.controls.push_back(ProfileControl{"a.knob", ControlKind::Knob, EncoderMode::Absolute, false});
    p.layout.push_back(ProfileGroup{"G", "ink", {"a.knob", "b.knob"}, 0});
    const auto faults = validate_profile(p);
    CHECK_EQ(faults.size(), std::size_t{1});
    CHECK(faults[0].find("b.knob") != std::string::npos);
}

SVJ_TEST("profile: duplicate ids and relative pots are faults") {
    DeviceProfile p;
    p.name = "x";
    p.controls.push_back(ProfileControl{"k", ControlKind::Knob, EncoderMode::Absolute, false});
    p.controls.push_back(ProfileControl{"k", ControlKind::Knob, EncoderMode::Absolute, false});
    p.controls.push_back(ProfileControl{"pot", ControlKind::Knob, EncoderMode::Relative64, false});
    const auto faults = validate_profile(p);
    CHECK_EQ(faults.size(), std::size_t{2});
}

SVJ_TEST("profile: the targets are the controls, in order, and the Elite checklist is them") {
    const DeviceProfile& elite = builtin_elite();
    const auto targets = profile_targets(elite);
    CHECK_EQ(targets.size(), elite.controls.size());
    CHECK_EQ(targets[0].id, elite.controls[0].id);
    const auto layout = elite_layout();
    CHECK_EQ(layout.size(), targets.size());
    CHECK_EQ(layout.back().id, targets.back().id);
}

SVJ_TEST("profile: declaring a profile carries the encoder mode onto the surface") {
    Surface surface;
    declare_profile(builtin_elite(), surface);
    const ControlIndex browse = surface.find("browse.encoder");
    CHECK(browse != kNoControl);
    CHECK(surface.at(browse).mode == EncoderMode::Relative64);
    CHECK(surface.at(surface.find("ch1.fader")).mode == EncoderMode::Absolute);
}

SVJ_TEST("profile: shipped bindings land on the device they are instantiated for") {
    DeviceProfile p;
    p.name = "x";
    p.controls.push_back(ProfileControl{"k", ControlKind::Knob, EncoderMode::Absolute, false});
    ProfileBinding b;
    b.id = "k";
    b.address.kind = MidiKind::ControlChange;
    b.address.channel = 0;
    b.address.number = 7;
    p.bindings.push_back(b);

    Surface surface;
    bind_profile(p, 2, surface);
    MidiEvent ev;
    ev.kind = MidiKind::ControlChange;
    ev.channel = 0;
    ev.number = 7;
    ev.value = 100;
    ev.device = 2;
    CHECK(surface.apply(ev, 1) == surface.find("k"));
    ev.device = 0;  // the same CC from the mixer is not this knob
    CHECK_EQ(surface.apply(ev, 2), kNoControl);
}

SVJ_TEST("profile: the port match with an ordinal picks the second turntable") {
    // Windows lists the pair as "RP8000mk2" and "2 - RP8000mk2"; both contain
    // the fragment, and which is left is the desk's business.
    const std::vector<std::string> ports = {"FromMax", "MOTU Pro Audio Midi In", "ELITE",
                                            "RP8000mk2", "2 - RP8000mk2"};
    CHECK_EQ(match_port("RP8000", 0, ports), 3);
    CHECK_EQ(match_port("RP8000", 1, ports), 4);
    CHECK_EQ(match_port("RP8000", 2, ports), -1);
    CHECK_EQ(match_port("elite", 0, ports), 2);  // case-insensitive
    CHECK_EQ(match_port("APC40", 0, ports), -1);
    CHECK_EQ(match_port("", 0, ports), -1);
}

SVJ_TEST("profile: the RP-8000 profile for deck B names its pads after B, three layers deep") {
    const DeviceProfile b = rp8000_profile('b');
    CHECK(has(b, "pad.rp8000.b.l1.1"));
    CHECK(has(b, "pad.rp8000.b.l3.8"));
    CHECK(!has(b, "pad.rp8000.a.l1.1"));
    CHECK_EQ(b.controls.size(), std::size_t{24});
    CHECK_EQ(rp8000_layout('b', 2).size(), std::size_t{8});
}

SVJ_TEST("profile: the default rig declares both turntables on all three layers") {
    const auto rig = default_rig_layout();
    std::set<std::string> ids;
    for (const LearnTarget& t : rig) ids.insert(t.id);
    CHECK(ids.count("pad.rp8000.a.l3.8") == 1);
    CHECK(ids.count("pad.rp8000.b.l2.1") == 1);
    CHECK_EQ(ids.size(), rig.size());  // and no id twice
}

SVJ_TEST("profile: no two sections of a measured panel overlap") {
    // A panel whose groups overlap draws one section on top of another, and
    // the controls underneath become unclickable -- a fault a screenshot can
    // easily hide, because the picture still looks like a mixer.
    for (const DeviceProfile* p : builtin_profiles()) {
        if (!profile_has_geometry(*p)) continue;
        for (std::size_t i = 0; i < p->layout.size(); ++i) {
            for (std::size_t j = i + 1; j < p->layout.size(); ++j) {
                const ProfileGroup& a = p->layout[i];
                const ProfileGroup& b = p->layout[j];
                if (a.front_panel || b.front_panel) continue;
                const bool apart = a.x + a.w <= b.x + 0.01f || b.x + b.w <= a.x + 0.01f ||
                                   a.y + a.h <= b.y + 0.01f || b.y + b.h <= a.y + 0.01f;
                if (!apart) {
                    std::fprintf(stderr, "« %s » recouvre « %s »\n", a.title.c_str(),
                                 b.title.c_str());
                }
                CHECK(apart);
            }
        }
    }
}

SVJ_TEST("profile: every control of a measured panel is drawn somewhere, or deliberately not") {
    // A control that is declared and never drawn is learnable and invisible.
    // The two encoder pushes are the exception, on purpose: they are the same
    // physical part as the encoder above them.
    const DeviceProfile& elite = builtin_elite();
    std::set<std::string> drawn;
    for (const ProfileGroup& g : elite.layout) {
        for (const std::string& id : g.controls) drawn.insert(id);
    }
    for (const ProfileControl& c : elite.controls) {
        if (c.id.find(".push") != std::string::npos) continue;
        if (drawn.count(c.id) == 0) std::fprintf(stderr, "non dessiné : %s\n", c.id.c_str());
        CHECK(drawn.count(c.id) == 1);
    }
}
