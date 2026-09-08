#include <cstdio>

#include "config/mapping_io.h"
#include "harness.h"

using namespace svj;

namespace {

SurfaceConfig sample() {
    SurfaceConfig config;

    LearnResult xfader;
    xfader.id = "xfader";
    xfader.kind = ControlKind::Fader;
    xfader.address.kind = MidiKind::ControlChange;
    xfader.address.channel = 0;
    xfader.address.number = 7;
    config.bindings.push_back(xfader);

    LearnResult pad;
    pad.id = "pad.elite.a.1";
    pad.kind = ControlKind::Pad;
    pad.address.kind = MidiKind::NoteOn;
    pad.address.channel = 2;
    pad.address.number = 40;
    config.bindings.push_back(pad);

    Mapping yaw;
    yaw.name = "hi knob to 360 yaw";
    yaw.source.kind = SourceKind::Control;
    yaw.source.control_id = "ch1.eq.hi";
    yaw.transform.curve = CurveKind::SCurve;
    yaw.transform.shape = 2.5f;
    yaw.transform.out_lo = -180.0f;
    yaw.transform.out_hi = 180.0f;
    yaw.transform.deadzone = 0.04f;
    yaw.transform.smoothing_ms = 40.0f;
    yaw.destination.kind = DestinationKind::Local;
    yaw.destination.target = "deck.a.yaw";
    config.mappings.push_back(yaw);

    Mapping shake;
    shake.name = "scratch rate to camera shake";
    shake.source.kind = SourceKind::DeckScratchRate;
    shake.source.deck = 1;
    shake.transform.in_hi = 12.0f;
    shake.transform.invert = true;
    shake.destination.kind = DestinationKind::Osc;
    shake.destination.target = "/ue/shake";
    config.mappings.push_back(shake);

    return config;
}

}  // namespace

SVJ_TEST("config: a configuration survives a JSON round trip") {
    const SurfaceConfig original = sample();
    SurfaceConfig decoded;
    std::string error;
    CHECK(config_from_json(config_to_json(original), decoded, error));
    CHECK_EQ(error, std::string());

    CHECK_EQ(decoded.bindings.size(), std::size_t{2});
    CHECK_EQ(decoded.bindings[0].id, std::string("xfader"));
    CHECK(decoded.bindings[0].kind == ControlKind::Fader);
    CHECK_EQ(int(decoded.bindings[0].address.number), 7);
    CHECK(decoded.bindings[1].address.kind == MidiKind::NoteOn);
    CHECK_EQ(int(decoded.bindings[1].address.channel), 2);

    CHECK_EQ(decoded.mappings.size(), std::size_t{2});
    const Mapping& yaw = decoded.mappings[0];
    CHECK_EQ(yaw.name, std::string("hi knob to 360 yaw"));
    CHECK(yaw.source.kind == SourceKind::Control);
    CHECK_EQ(yaw.source.control_id, std::string("ch1.eq.hi"));
    CHECK(yaw.transform.curve == CurveKind::SCurve);
    CHECK_NEAR(yaw.transform.shape, 2.5, 1e-6);
    CHECK_NEAR(yaw.transform.out_lo, -180.0, 1e-6);
    CHECK_NEAR(yaw.transform.deadzone, 0.04, 1e-6);
    CHECK(yaw.destination.kind == DestinationKind::Local);

    const Mapping& shake = decoded.mappings[1];
    CHECK(shake.source.kind == SourceKind::DeckScratchRate);
    CHECK_EQ(int(shake.source.deck), 1);
    CHECK(shake.transform.invert);
    CHECK(shake.destination.kind == DestinationKind::Osc);
    CHECK_EQ(shake.destination.target, std::string("/ue/shake"));
}

SVJ_TEST("config: enums are written as names so the file stays hand-editable") {
    const std::string json = config_to_json(sample());
    CHECK(json.find("\"scurve\"") != std::string::npos);
    CHECK(json.find("\"deck.scratch_rate\"") != std::string::npos);
    CHECK(json.find("\"osc\"") != std::string::npos);
    CHECK(json.find("\"fader\"") != std::string::npos);
}

SVJ_TEST("config: malformed JSON is reported rather than silently ignored") {
    SurfaceConfig out;
    std::string error;
    CHECK(!config_from_json("{ not json", out, error));
    CHECK(!error.empty());
}

SVJ_TEST("config: a non-object document is rejected") {
    SurfaceConfig out;
    std::string error;
    CHECK(!config_from_json("[1, 2, 3]", out, error));
    CHECK(error.find("object") != std::string::npos);
}

SVJ_TEST("config: an unknown enum names both the value and its field") {
    SurfaceConfig out;
    std::string error;
    const char* text = R"({"mappings":[{"transform":{"curve":"banana"}}]})";
    CHECK(!config_from_json(text, out, error));
    CHECK(error.find("banana") != std::string::npos);
    CHECK(error.find("curve") != std::string::npos);
}

SVJ_TEST("config: a control source with no id is rejected") {
    SurfaceConfig out;
    std::string error;
    const char* text = R"({"mappings":[{"name":"broken","source":{"kind":"control"}}]})";
    CHECK(!config_from_json(text, out, error));
    CHECK(error.find("broken") != std::string::npos);
}

SVJ_TEST("config: absent fields keep their defaults") {
    SurfaceConfig out;
    std::string error;
    CHECK(config_from_json(R"({"mappings":[{"name":"bare"}]})", out, error));
    CHECK_EQ(out.mappings.size(), std::size_t{1});
    const Transform& t = out.mappings[0].transform;
    CHECK(t.curve == CurveKind::Linear);
    CHECK_NEAR(t.in_hi, 1.0, 1e-6);
    CHECK(out.mappings[0].enabled);
}

SVJ_TEST("config: an empty document loads as an empty configuration") {
    SurfaceConfig out;
    std::string error;
    CHECK(config_from_json("{}", out, error));
    CHECK(out.bindings.empty());
    CHECK(out.mappings.empty());
}

SVJ_TEST("config: a failed parse leaves the destination untouched") {
    SurfaceConfig out = sample();
    std::string error;
    CHECK(!config_from_json("{ bad", out, error));
    CHECK_EQ(out.bindings.size(), std::size_t{2});  // still the previous contents
}

SVJ_TEST("config: applying a configuration declares and binds its controls") {
    Surface surface;
    config_apply(sample(), surface);
    CHECK_EQ(surface.size(), std::size_t{2});

    const ControlIndex xfader = surface.find("xfader");
    CHECK(xfader != kNoControl);

    MidiEvent event;
    event.kind = MidiKind::ControlChange;
    event.number = 7;
    event.value = 127;
    CHECK_EQ(surface.apply(event, 1), xfader);
}

SVJ_TEST("config: saving then loading a file preserves the configuration") {
    const std::string path = "svj_config_roundtrip.json";
    std::string error;
    CHECK(config_save(sample(), path, error));

    SurfaceConfig loaded;
    CHECK(config_load(path, loaded, error));
    CHECK_EQ(loaded.mappings.size(), std::size_t{2});
    CHECK_EQ(loaded.bindings[1].id, std::string("pad.elite.a.1"));
    std::remove(path.c_str());
}

SVJ_TEST("config: loading a missing file reports the path") {
    SurfaceConfig out;
    std::string error;
    CHECK(!config_load("definitely_not_here.json", out, error));
    CHECK(error.find("definitely_not_here.json") != std::string::npos);
}

// --- the desk's settings ------------------------------------------------------

#include "config/settings_io.h"

SVJ_TEST("settings: a round trip through JSON preserves the desk") {
    DeskSettings desk;
    desk.platter_endpoint = "Elite";
    desk.platter_first_channel = 2;
    desk.carrier_hz = 2000.0;

    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK_EQ(back.platter_endpoint, std::string("Elite"));
    CHECK_EQ(back.platter_first_channel, 2u);
    CHECK_NEAR(back.carrier_hz, 2000.0, 1e-12);
}

SVJ_TEST("settings: a missing file is the measured desk, not an error") {
    // The defaults ARE the desk the carrier was calibrated on (docs/cablage.md),
    // so a fresh checkout there simply works with no file at all.
    DeskSettings desk;
    desk.platter_endpoint = "should be replaced";
    std::string error;
    CHECK(settings_load("this/file/does/not/exist.json", desk, error));
    CHECK_EQ(desk.platter_endpoint, std::string("MOTU"));
    CHECK_EQ(desk.platter_first_channel, 4u);
    CHECK_NEAR(desk.carrier_hz, 1000.0, 1e-12);
}

SVJ_TEST("settings: a broken file names the field and leaves the desk untouched") {
    // Half-applying would open the right input on the wrong channel pair with
    // total confidence, which is worse than refusing.
    DeskSettings desk;
    desk.platter_endpoint = "keep";
    std::string error;
    CHECK(!settings_from_json(R"({"platter": {"endpoint": "X", "carrier_hz": -5}})",
                              desk, error));
    CHECK(error.find("carrier_hz") != std::string::npos);
    CHECK_EQ(desk.platter_endpoint, std::string("keep"));
}

SVJ_TEST("settings: fields left out keep their defaults") {
    DeskSettings desk;
    std::string error;
    CHECK(settings_from_json(R"({"platter": {"first_channel": 6}})", desk, error));
    CHECK_EQ(desk.platter_first_channel, 6u);
    CHECK_EQ(desk.platter_endpoint, std::string("MOTU"));
    CHECK_NEAR(desk.carrier_hz, 1000.0, 1e-12);
}

SVJ_TEST("settings: library folders and cache dir survive a round trip") {
    DeskSettings desk;
    desk.library_folders = {"D:/rushes", "E:/loops/2026"};
    desk.cache_dir = "D:/svcache";
    desk.sequence_fps = 24.0;
    desk.analysis_max_width = 1920;

    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK_EQ(back.library_folders.size(), std::size_t{2});
    CHECK_EQ(back.library_folders[1], std::string("E:/loops/2026"));
    CHECK_EQ(back.cache_dir, std::string("D:/svcache"));
    CHECK_NEAR(back.sequence_fps, 24.0, 1e-12);
    CHECK_EQ(back.analysis_max_width, 1920u);
}

SVJ_TEST("settings: a library section left out keeps the clips folder default") {
    // The analysis pass has always left caches in clips/; a settings file
    // written before the library section existed must not orphan them.
    DeskSettings desk;
    std::string error;
    CHECK(settings_from_json(R"({"midi": {"port": "APC"}})", desk, error));
    CHECK_EQ(desk.library_folders.size(), std::size_t{1});
    CHECK_EQ(desk.library_folders[0], std::string("clips"));
    CHECK(desk.cache_dir.empty());
}

SVJ_TEST("settings: folders that are not a list of paths name the field") {
    DeskSettings desk;
    std::string error;
    CHECK(!settings_from_json(R"({"library": {"folders": "D:/rushes"}})", desk, error));
    CHECK(error.find("library.folders") != std::string::npos);
    CHECK(!settings_from_json(R"({"library": {"folders": ["ok", 7]}})", desk, error));
    CHECK(error.find("library.folders") != std::string::npos);
}

SVJ_TEST("config: a binding without a device is device 0, the mixer") {
    // Files written before the rig had more than one port say nothing about
    // the device; they meant the mixer, and must keep meaning it.
    SurfaceConfig out;
    std::string error;
    CHECK(config_from_json(
        R"({"bindings": [{"id": "xfader", "kind": "fader", "midi": {"type": "cc", "channel": 0, "number": 7}}]})",
        out, error));
    CHECK_EQ(static_cast<int>(out.bindings[0].address.device), 0);
    out.bindings[0].address.device = 2;
    SurfaceConfig back;
    CHECK(config_from_json(config_to_json(out), back, error));
    CHECK_EQ(static_cast<int>(back.bindings[0].address.device), 2);
}

SVJ_TEST("config: modulator and audio-band sources survive a round trip") {
    // They used to come back as "control" mappings with no control: the name
    // table had seven of the nine kinds.
    SurfaceConfig config;
    Mapping lfo;
    lfo.name = "lfo";
    lfo.source.kind = SourceKind::Modulator;
    lfo.source.index = 1;
    lfo.destination.target = "fx.1.mix";
    config.mappings.push_back(lfo);
    Mapping band;
    band.name = "band";
    band.source.kind = SourceKind::AudioBand;
    band.source.index = 3;
    band.destination.target = "fx.a.bloom.amount";
    config.mappings.push_back(band);

    SurfaceConfig back;
    std::string error;
    CHECK(config_from_json(config_to_json(config), back, error));
    CHECK(back.mappings[0].source.kind == SourceKind::Modulator);
    CHECK_EQ(static_cast<int>(back.mappings[0].source.index), 1);
    CHECK(back.mappings[1].source.kind == SourceKind::AudioBand);
    CHECK_EQ(static_cast<int>(back.mappings[1].source.index), 3);
}

SVJ_TEST("settings: the rig's devices and the mixer's switches round-trip") {
    DeskSettings desk;
    desk.devices = {DeviceSetting{"reloop_elite", "ELITE", 0, 'a'},
                    DeviceSetting{"apc40_mk2", "APC40", 0, 'a'},
                    DeviceSetting{"rp8000", "RP8000", 1, 'b'}};
    desk.mix.xfader = FaderCurve::Cut;
    desk.mix.channel = FaderCurve::Smooth;
    desk.mix.xfader_reverse = true;
    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK_EQ(back.devices.size(), std::size_t{3});
    CHECK_EQ(back.devices[1].profile, std::string("apc40_mk2"));
    CHECK_EQ(back.devices[2].ordinal, 1);
    CHECK_EQ(back.devices[2].deck, 'b');
    CHECK(back.mix.xfader == FaderCurve::Cut);
    CHECK(back.mix.channel == FaderCurve::Smooth);
    CHECK(back.mix.xfader_reverse);
    CHECK(!back.mix.channel_reverse);
}

SVJ_TEST("settings: a file naming only midi.port is one device, the mixer, on that port") {
    DeskSettings desk;
    std::string error;
    CHECK(settings_from_json(R"({"midi": {"port": "APC"}})", desk, error));
    CHECK_EQ(desk.devices.size(), std::size_t{1});
    CHECK_EQ(desk.devices[0].port, std::string("APC"));
    CHECK_EQ(desk.devices[0].profile, std::string("reloop_elite"));
}

SVJ_TEST("settings: an unknown curve word names the field") {
    DeskSettings desk;
    std::string error;
    CHECK(!settings_from_json(R"({"mix": {"xfader_curve": "steep"}})", desk, error));
    CHECK(error.find("xfader_curve") != std::string::npos);
}

SVJ_TEST("settings: the output screen is remembered by NAME, not by index") {
    // Display indices are renumbered whenever something is plugged in. A set
    // that opens on the wrong screen because a hub enumerated differently is
    // worse than one that opens on none, so the name is what is kept.
    DeskSettings desk;
    desk.output_display = "DELL U2720Q";
    desk.output_open = true;
    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK_EQ(back.output_display, std::string("DELL U2720Q"));
    CHECK(back.output_open);
}

SVJ_TEST("settings: no output section means no output screen") {
    DeskSettings desk;
    std::string error;
    CHECK(settings_from_json(R"({"midi": {"port": "ELITE"}})", desk, error));
    CHECK(desk.output_display.empty());
    CHECK(!desk.output_open);
}

SVJ_TEST("settings: a malformed output section names the field") {
    DeskSettings desk;
    std::string error;
    CHECK(!settings_from_json(R"({"output": {"display": 3}})", desk, error));
    CHECK(error.find("output.display") != std::string::npos);
}

SVJ_TEST("settings: the VRAM budget and the Unreal endpoint survive a round trip") {
    DeskSettings desk;
    desk.vram_budget_mb = 1024;
    desk.control_host = "192.168.1.40";
    desk.control_port = 9000;
    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK_EQ(back.vram_budget_mb, 1024u);
    CHECK_EQ(back.control_host, std::string("192.168.1.40"));
    CHECK_EQ(back.control_port, 9000u);
}

SVJ_TEST("settings: a file from before these fields keeps their defaults") {
    DeskSettings back;
    std::string error;
    CHECK(settings_from_json("{\"version\": 1}", back, error));
    CHECK_EQ(back.vram_budget_mb, 256u);
    CHECK_EQ(back.control_port, 7331u);
    CHECK_EQ(back.control_host, std::string("127.0.0.1"));
}

SVJ_TEST("settings: an impossible port or budget is named, not silently clamped") {
    DeskSettings back;
    std::string error;
    CHECK(!settings_from_json("{\"unreal\": {\"port\": 70000}}", back, error));
    CHECK(error.find("port") != std::string::npos);
    CHECK(!settings_from_json("{\"engine\": {\"vram_budget_mb\": 4}}", back, error));
    CHECK(error.find("vram") != std::string::npos);
}

SVJ_TEST("settings: each channel keeps its own curve, and an old file gives both the same") {
    DeskSettings desk;
    desk.mix.channel = FaderCurve::Cut;
    desk.mix.channel_b = FaderCurve::Smooth;
    desk.mix.channel_b_reverse = true;
    DeskSettings back;
    std::string error;
    CHECK(settings_from_json(settings_to_json(desk), back, error));
    CHECK(back.mix.channel == FaderCurve::Cut);
    CHECK(back.mix.channel_b == FaderCurve::Smooth);
    CHECK(back.mix.channel_b_reverse);
    CHECK(!back.mix.channel_reverse);

    DeskSettings old;
    CHECK(settings_from_json("{\"mix\": {\"fader_curve\": \"cut\", \"fader_reverse\": true}}", old, error));
    CHECK(old.mix.channel_b == FaderCurve::Cut);
    CHECK(old.mix.channel_b_reverse);
}
