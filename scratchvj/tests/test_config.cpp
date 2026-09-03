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
