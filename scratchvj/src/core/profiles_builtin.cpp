// scratchvj — the two devices this instrument was built around, as tables.
//
// In C++ rather than JSON so core stays free of a parser and the tests need
// no files. Nothing here is a CC number: the Elite's map is not published and
// was measured to be learnable (41 controls in one sweep); the bindings come
// from --midi-learn and live in mapping.json.
//
// THE GEOMETRY IS MEASURED, not sketched. The positions below come from
// Reloop's own top-panel render (238128_Reloop_TP.jpg) and front view, read
// pixel-wise and cross-checked against the callout diagram in the official
// 44-page manual (rev 1.4, sections A-G). They are normalised there -- x 0 at
// the left edge, y 0 at the REAR edge -- and converted here to the panel's
// real millimetres, 290 wide by 400 deep. That matters: a control surface
// drawn as a list is a second panel to learn, while one drawn where the
// hardware has it is a mirror of what is under the hand.
//
// Four things the measurement corrected, each of which had been guessed wrong:
//
//   - The Elite has NO channel cue/PFL buttons. Monitoring is a cue channel
//     SLIDER in the centre (manual B17), blended with CUE MIX.
//   - The input selector is on the top panel, INBOARD of the gain knob beside
//     the master, and it is a four-way slide: USB A / PHONO / LINE / USB B.
//   - There is one SHIFT, in the centre, not one per side.
//   - The front edge carries THREE curve-and-reverse pairs, not two: channel
//     1, the crossfader, and channel 2, in that left-to-right order.
//
// The front-edge controls are declared `front_panel`: they exist, and a
// top-down picture is exactly where they are not, so the drawing gives them
// their own strip. Whether any of them emits MIDI is still unmeasured, which
// is why they are also `optional` -- learn passes over them rather than
// leaving a sweep unfinishable.
#include "core/profile.h"

namespace svj {
namespace {

// The panel, in millimetres. Reloop's spec is 290 x 426 x 105 mm; the 426
// includes the rear jack overhang and the front lip, so the flat top is
// about 290 x 400.
constexpr float kEliteW = 290.0f;
constexpr float kEliteH = 400.0f;

float px(float normalised) { return normalised * kEliteW; }
float py(float normalised) { return normalised * kEliteH; }

void add(DeviceProfile& p, std::string id, ControlKind kind,
         EncoderMode mode = EncoderMode::Absolute, bool optional = false) {
    p.controls.push_back(ProfileControl{std::move(id), kind, mode, optional});
}

// A placed group, in the render's normalised coordinates.
void group_at(DeviceProfile& p, std::string title, std::string accent,
              std::vector<std::string> ids, int columns, float x0, float y0, float x1,
              float y1) {
    ProfileGroup g;
    g.title = std::move(title);
    g.accent = std::move(accent);
    g.controls = std::move(ids);
    g.columns = columns;
    g.x = px(x0);
    g.y = py(y0);
    g.w = px(x1 - x0);
    g.h = py(y1 - y0);
    p.layout.push_back(std::move(g));
}

void front_group(DeviceProfile& p, std::string title, std::vector<std::string> ids) {
    ProfileGroup g;
    g.title = std::move(title);
    g.accent = "faint";
    g.controls = std::move(ids);
    g.front_panel = true;
    p.layout.push_back(std::move(g));
}

// One channel strip. Ids are hierarchical so the UI can group them without a
// separate table of which control belongs where. "filter" is the Tweak FX
// knob -- Filter is its default mode and the id the mappings have used since
// the first commit. The EQ is a THREE-band isolator with full kill (70 Hz,
// 1 kHz, 13 kHz); sources claiming four bands are wrong.
void channel(DeviceProfile& p, int n) {
    const std::string c = "ch" + std::to_string(n) + ".";
    add(p, c + "trim", ControlKind::Knob);
    add(p, c + "eq.hi", ControlKind::Knob);
    add(p, c + "eq.mid", ControlKind::Knob);
    add(p, c + "eq.low", ControlKind::Knob);
    add(p, c + "filter", ControlKind::Knob);        // the Tweak FX knob
    add(p, c + "tweak.mode", ControlKind::Button);  // inboard, beside the centre
    add(p, c + "input", ControlKind::Knob);         // 4-way slide: USB A/PHONO/LINE/USB B
    add(p, c + "fader", ControlKind::Fader);
}

void fx_unit(DeviceProfile& p, char side) {
    const std::string f = std::string("fx.") + side + ".";
    add(p, f + "select.1", ControlKind::Button);
    add(p, f + "select.2", ControlKind::Button);
    add(p, f + "select.3", ControlKind::Button);
    add(p, f + "level", ControlKind::Fader);  // a vertical mini-slider, not a knob
    add(p, f + "beats", ControlKind::Encoder, EncoderMode::Relative64);
    add(p, f + "on", ControlKind::Button);
    add(p, f + "hold", ControlKind::Button);
    add(p, f + "sampler", ControlKind::Button);  // SMP FX: routes sampler/aux into the unit
}

void pads(DeviceProfile& p, const std::string& prefix, int count) {
    for (int i = 1; i <= count; ++i) add(p, prefix + std::to_string(i), ControlKind::Pad);
}

std::vector<std::string> ids(const std::string& prefix, int from, int to) {
    std::vector<std::string> out;
    for (int i = from; i <= to; ++i) out.push_back(prefix + std::to_string(i));
    return out;
}

DeviceProfile make_elite() {
    DeviceProfile p;
    p.name = "reloop_elite";
    p.display_name = "Reloop Elite";
    p.port_hint = "ELITE";
    p.verified = true;  // roster and geometry from Reloop's own render and manual
    p.panel_w = kEliteW;
    p.panel_h = kEliteH;

    channel(p, 1);
    channel(p, 2);
    add(p, "xfader", ControlKind::Fader);

    fx_unit(p, 'a');
    fx_unit(p, 'b');

    for (const char side : {'a', 'b'}) {
        const std::string l = std::string("loop.") + side + ".";
        add(p, l + "encoder", ControlKind::Encoder, EncoderMode::Relative64);
        add(p, l + "encoder.push", ControlKind::Button);
        add(p, l + "inout", ControlKind::Button);  // manual loop in / out, yellow
    }

    // Sixteen RGB pads, eight a side as four columns by two rows, with the
    // four mode buttons in a row directly BELOW them (Hot Cue, Loop Roll,
    // Slicer, Sampler -- three legend layers make the twelve modes) and the
    // parameter pair pushed to the outer corner.
    pads(p, "pad.elite.a.", 8);
    pads(p, "pad.elite.b.", 8);
    for (const char side : {'a', 'b'}) {
        const std::string m = std::string("pad.elite.") + side + ".mode.";
        for (int i = 1; i <= 4; ++i) add(p, m + std::to_string(i), ControlKind::Button);
        add(p, std::string("pad.elite.") + side + ".param.minus", ControlKind::Button);
        add(p, std::string("pad.elite.") + side + ".param.plus", ControlKind::Button);
    }
    add(p, "shift", ControlKind::Button);  // one, in the centre

    add(p, "browse.encoder", ControlKind::Encoder, EncoderMode::Relative64);
    add(p, "browse.encoder.push", ControlKind::Button);
    add(p, "browse.load.a", ControlKind::Button);
    add(p, "browse.load.b", ControlKind::Button);
    add(p, "browse.back", ControlKind::Button);

    add(p, "master.level", ControlKind::Knob);
    add(p, "booth.level", ControlKind::Knob);
    add(p, "booth.mono", ControlKind::Button);
    add(p, "cue.level", ControlKind::Knob);
    add(p, "cue.mix", ControlKind::Knob);
    add(p, "cue.select", ControlKind::Fader);  // B17: which channel is monitored

    // --- the front edge ------------------------------------------------------
    // Left to right as the manual's section F lists them. Three curve and
    // reverse pairs: channel 1, the crossfader BETWEEN them, channel 2.
    add(p, "mic.level", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "mic.echo", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "mic.eq.hi", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "mic.eq.low", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "mic.mode", ControlKind::Button, EncoderMode::Absolute, true);
    add(p, "fader.a.curve", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "fader.a.reverse", ControlKind::Button, EncoderMode::Absolute, true);
    add(p, "xfader.curve", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "xfader.reverse", ControlKind::Button, EncoderMode::Absolute, true);
    add(p, "fader.b.curve", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "fader.b.reverse", ControlKind::Button, EncoderMode::Absolute, true);
    add(p, "sampler.level", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "aux.level", ControlKind::Knob, EncoderMode::Absolute, true);
    add(p, "cue.split", ControlKind::Button, EncoderMode::Absolute, true);

    // --- where every one of them sits ---------------------------------------
    // Left wing: the Serato FX unit, then the loop section under it.
    group_at(p, "FX 1", "amber", {"fx.a.select.1", "fx.a.select.2", "fx.a.select.3"}, 3,
             0.040f, 0.105f, 0.262f, 0.172f);
    group_at(p, "", "amber", {"fx.a.level", "fx.a.beats"}, 2, 0.050f, 0.176f, 0.258f, 0.278f);
    group_at(p, "", "amber", {"fx.a.on", "fx.a.hold"}, 2, 0.050f, 0.282f, 0.258f, 0.330f);
    group_at(p, "LOOP 1", "amber", {"loop.a.encoder", "loop.a.encoder.push", "loop.a.inout"}, 3,
             0.040f, 0.352f, 0.262f, 0.440f);

    // Right wing, mirrored about the centre line.
    group_at(p, "FX 2", "slate", {"fx.b.select.1", "fx.b.select.2", "fx.b.select.3"}, 3,
             0.738f, 0.105f, 0.960f, 0.172f);
    group_at(p, "", "slate", {"fx.b.beats", "fx.b.level"}, 2, 0.742f, 0.176f, 0.950f, 0.278f);
    group_at(p, "", "slate", {"fx.b.hold", "fx.b.on"}, 2, 0.742f, 0.282f, 0.950f, 0.330f);
    group_at(p, "LOOP 2", "slate", {"loop.b.inout", "loop.b.encoder.push", "loop.b.encoder"}, 3,
             0.738f, 0.352f, 0.960f, 0.440f);

    // The two channel strips: gain, the three-band isolator, the Tweak knob.
    group_at(p, "VOIE 1", "amber",
             {"ch1.trim", "ch1.eq.hi", "ch1.eq.mid", "ch1.eq.low", "ch1.filter"}, 1, 0.272f,
             0.028f, 0.362f, 0.440f);
    group_at(p, "VOIE 2", "slate",
             {"ch2.trim", "ch2.eq.hi", "ch2.eq.mid", "ch2.eq.low", "ch2.filter"}, 1, 0.638f,
             0.028f, 0.728f, 0.440f);

    // The centre column, rear to front.
    group_at(p, "", "ink", {"ch1.input", "master.level", "ch2.input"}, 3, 0.368f, 0.028f, 0.632f,
             0.100f);
    group_at(p, "", "ink", {"fx.a.sampler", "booth.mono", "fx.b.sampler"}, 3, 0.368f, 0.104f,
             0.632f, 0.128f);
    group_at(p, "", "ink", {"booth.level"}, 1, 0.452f, 0.132f, 0.548f, 0.190f);
    // The encoders' pushes are the same physical control as the encoder, so
    // they are declared (learn covers them) but have no place of their own.
    group_at(p, "BROWSE", "ink", {"browse.load.a", "browse.encoder", "browse.load.b"}, 3, 0.368f,
             0.194f, 0.632f, 0.262f);
    group_at(p, "", "ink", {"cue.level", "browse.back", "cue.mix"}, 3, 0.368f, 0.266f, 0.632f,
             0.318f);
    group_at(p, "CUE", "ink", {"cue.select"}, 1, 0.420f, 0.322f, 0.580f, 0.372f);
    group_at(p, "", "ink", {"ch1.tweak.mode", "shift", "ch2.tweak.mode"}, 3, 0.368f, 0.376f,
             0.632f, 0.440f);

    // The pad field, wider than the mixer core above it.
    group_at(p, "PADS 1", "amber", ids("pad.elite.a.", 1, 8), 4, 0.048f, 0.442f, 0.472f, 0.604f);
    group_at(p, "PADS 2", "slate", ids("pad.elite.b.", 1, 8), 4, 0.528f, 0.442f, 0.952f, 0.604f);
    group_at(p, "", "amber", ids("pad.elite.a.mode.", 1, 4), 4, 0.048f, 0.616f, 0.472f, 0.662f);
    group_at(p, "", "slate", ids("pad.elite.b.mode.", 1, 4), 4, 0.528f, 0.616f, 0.952f, 0.662f);
    group_at(p, "", "amber", {"pad.elite.a.param.minus", "pad.elite.a.param.plus"}, 2, 0.048f,
             0.666f, 0.160f, 0.702f);
    group_at(p, "", "slate", {"pad.elite.b.param.minus", "pad.elite.b.param.plus"}, 2, 0.840f,
             0.666f, 0.952f, 0.702f);

    // The faders, and the crossfader alone at the front.
    group_at(p, "", "amber", {"ch1.fader"}, 1, 0.278f, 0.700f, 0.366f, 0.840f);
    group_at(p, "", "slate", {"ch2.fader"}, 1, 0.634f, 0.700f, 0.722f, 0.840f);
    group_at(p, "CROSSFADER", "ink", {"xfader"}, 1, 0.380f, 0.880f, 0.620f, 0.960f);

    // The front edge, in its own strip under the panel.
    front_group(p, "MIC", {"mic.level", "mic.echo", "mic.eq.hi", "mic.eq.low", "mic.mode"});
    front_group(p, "COURBES", {"fader.a.curve", "fader.a.reverse", "xfader.curve",
                               "xfader.reverse", "fader.b.curve", "fader.b.reverse"});
    front_group(p, "SAMPLER / AUX / CASQUE", {"sampler.level", "aux.level", "cue.split"});
    return p;
}

DeviceProfile make_rp8000(char deck) {
    DeviceProfile p;
    p.name = "rp8000";
    p.display_name = "RP-8000 MK2";  // the deck letter is the rig's to add
    p.port_hint = "RP8000";
    p.verified = true;  // eight pads in three layers, per the manual
    // The layers are the SAME eight pads, switched on the turntable. Saying so
    // is not decoration: three groups of eight, drawn side by side with nothing
    // between them, read as twenty-four buttons -- which is how a panel for
    // this device got planned before anyone counted the pads.
    p.note = "les 3 couches sont les M\xC3\x8aMES 8 pads, commut\xC3\xA9s sur la platine";
    for (int layer = 1; layer <= 3; ++layer) {
        const std::string prefix =
            std::string("pad.rp8000.") + deck + ".l" + std::to_string(layer) + ".";
        pads(p, prefix, 8);
        // No geometry, and this one is a DECISION rather than a gap -- see the
        // roadmap. Two reasons. The panel model gives one identifier one place,
        // and a layer is device state the application cannot see, so 24 ids
        // cannot share 8 places without either overlapping sections or a claim
        // about which layer is live that nothing measures. And even solved, a
        // turntable panel would be nine tenths platter: the Elite's picture
        // earns its keep because a hand hunts one knob among forty in sections,
        // where these eight sit in a single strip and are already drawn four
        // across.
        ProfileGroup g;
        g.title = std::string("COUCHE ") + std::to_string(layer);
        g.accent = deck == 'a' ? "amber" : "slate";
        g.controls = ids(prefix, 1, 8);
        g.columns = 4;
        p.layout.push_back(std::move(g));
    }
    return p;
}

}  // namespace

const DeviceProfile& builtin_elite() {
    static const DeviceProfile profile = make_elite();
    return profile;
}

const DeviceProfile& builtin_rp8000() {
    static const DeviceProfile profile = make_rp8000('a');
    return profile;
}

DeviceProfile rp8000_profile(char deck) { return make_rp8000(deck == 'b' ? 'b' : 'a'); }

std::vector<const DeviceProfile*> builtin_profiles() {
    return {&builtin_elite(), &builtin_rp8000()};
}

const DeviceProfile* builtin_profile(std::string_view name) {
    for (const DeviceProfile* p : builtin_profiles()) {
        if (p->name == name) return p;
    }
    return nullptr;
}

}  // namespace svj
