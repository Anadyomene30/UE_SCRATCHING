#include "config/mapping_io.h"

#include <array>
#include <fstream>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace svj {
namespace {

using json = nlohmann::json;

// Enum name tables. Kept as explicit pairs so the file format does not silently
// change meaning when an enumerator is inserted in the middle.
constexpr std::array<std::pair<ControlKind, const char*>, 5> kControlKinds{{
    {ControlKind::Fader, "fader"},
    {ControlKind::Knob, "knob"},
    {ControlKind::Button, "button"},
    {ControlKind::Encoder, "encoder"},
    {ControlKind::Pad, "pad"},
}};

constexpr std::array<std::pair<MidiKind, const char*>, 5> kMidiKinds{{
    {MidiKind::NoteOn, "note"},
    {MidiKind::NoteOff, "note_off"},
    {MidiKind::ControlChange, "cc"},
    {MidiKind::PitchBend, "pitchbend"},
    {MidiKind::Other, "other"},
}};

constexpr std::array<std::pair<CurveKind, const char*>, 4> kCurveKinds{{
    {CurveKind::Linear, "linear"},
    {CurveKind::Exponential, "exp"},
    {CurveKind::Logarithmic, "log"},
    {CurveKind::SCurve, "scurve"},
}};

constexpr std::array<std::pair<SourceKind, const char*>, 7> kSourceKinds{{
    {SourceKind::Control, "control"},
    {SourceKind::DeckPosition, "deck.position"},
    {SourceKind::DeckVelocity, "deck.velocity"},
    {SourceKind::DeckAcceleration, "deck.acceleration"},
    {SourceKind::DeckScratchRate, "deck.scratch_rate"},
    {SourceKind::DeckConfidence, "deck.confidence"},
    {SourceKind::Gesture, "gesture"},
}};

constexpr std::array<std::pair<DestinationKind, const char*>, 2> kDestinationKinds{{
    {DestinationKind::Local, "local"},
    {DestinationKind::Osc, "osc"},
}};

template <typename Enum, std::size_t N>
const char* name_of(const std::array<std::pair<Enum, const char*>, N>& table, Enum value) {
    for (const auto& entry : table) {
        if (entry.first == value) return entry.second;
    }
    return table[0].second;
}

template <typename Enum, std::size_t N>
bool value_of(const std::array<std::pair<Enum, const char*>, N>& table, std::string_view name,
              Enum& out) {
    for (const auto& entry : table) {
        if (name == entry.second) {
            out = entry.first;
            return true;
        }
    }
    return false;
}

// Reads a named enum, reporting the field and the offending text on failure.
template <typename Enum, std::size_t N>
bool read_enum(const json& node, const char* field,
               const std::array<std::pair<Enum, const char*>, N>& table, Enum& out,
               std::string& error) {
    if (!node.contains(field)) return true;  // absent means keep the default
    if (!node[field].is_string()) {
        error = std::string("field '") + field + "' must be a string";
        return false;
    }
    const auto text = node[field].get<std::string>();
    if (!value_of(table, text, out)) {
        error = std::string("unknown value '") + text + "' for field '" + field + "'";
        return false;
    }
    return true;
}

float read_float(const json& node, const char* field, float fallback) {
    if (!node.contains(field) || !node[field].is_number()) return fallback;
    return node[field].get<float>();
}

json transform_to_json(const Transform& t) {
    return json{
        {"curve", name_of(kCurveKinds, t.curve)},
        {"shape", t.shape},
        {"in_lo", t.in_lo},
        {"in_hi", t.in_hi},
        {"out_lo", t.out_lo},
        {"out_hi", t.out_hi},
        {"invert", t.invert},
        {"deadzone", t.deadzone},
        {"smoothing_ms", t.smoothing_ms},
    };
}

bool transform_from_json(const json& node, Transform& t, std::string& error) {
    if (!read_enum(node, "curve", kCurveKinds, t.curve, error)) return false;
    t.shape = read_float(node, "shape", t.shape);
    t.in_lo = read_float(node, "in_lo", t.in_lo);
    t.in_hi = read_float(node, "in_hi", t.in_hi);
    t.out_lo = read_float(node, "out_lo", t.out_lo);
    t.out_hi = read_float(node, "out_hi", t.out_hi);
    t.deadzone = read_float(node, "deadzone", t.deadzone);
    t.smoothing_ms = read_float(node, "smoothing_ms", t.smoothing_ms);
    if (node.contains("invert") && node["invert"].is_boolean()) {
        t.invert = node["invert"].get<bool>();
    }
    return true;
}

}  // namespace

std::string config_to_json(const SurfaceConfig& config) {
    json root;
    root["version"] = 1;

    json bindings = json::array();
    for (const LearnResult& b : config.bindings) {
        bindings.push_back(json{
            {"id", b.id},
            {"kind", name_of(kControlKinds, b.kind)},
            {"midi", json{{"type", name_of(kMidiKinds, b.address.kind)},
                          {"channel", b.address.channel},
                          {"number", b.address.number}}},
        });
    }
    root["bindings"] = std::move(bindings);

    json mappings = json::array();
    for (const Mapping& m : config.mappings) {
        json source{{"kind", name_of(kSourceKinds, m.source.kind)}};
        if (m.source.kind == SourceKind::Control) {
            source["control"] = m.source.control_id;
        } else if (m.source.kind == SourceKind::Gesture) {
            source["gesture_bit"] = m.source.gesture_bit;
        }
        if (m.source.kind != SourceKind::Control && m.source.kind != SourceKind::Gesture) {
            source["deck"] = m.source.deck;
        }

        mappings.push_back(json{
            {"name", m.name},
            {"enabled", m.enabled},
            {"source", std::move(source)},
            {"transform", transform_to_json(m.transform)},
            {"destination", json{{"kind", name_of(kDestinationKinds, m.destination.kind)},
                                 {"target", m.destination.target}}},
        });
    }
    root["mappings"] = std::move(mappings);

    return root.dump(2);
}

bool config_from_json(std::string_view text, SurfaceConfig& out, std::string& error) {
    json root = json::parse(text, nullptr, false);
    if (root.is_discarded()) {
        error = "not valid JSON";
        return false;
    }
    if (!root.is_object()) {
        error = "top level must be an object";
        return false;
    }

    SurfaceConfig parsed;

    if (root.contains("bindings")) {
        if (!root["bindings"].is_array()) {
            error = "'bindings' must be an array";
            return false;
        }
        for (const json& node : root["bindings"]) {
            LearnResult b;
            if (!node.contains("id") || !node["id"].is_string()) {
                error = "a binding is missing its 'id'";
                return false;
            }
            b.id = node["id"].get<std::string>();
            if (!read_enum(node, "kind", kControlKinds, b.kind, error)) return false;

            if (node.contains("midi") && node["midi"].is_object()) {
                const json& midi = node["midi"];
                if (!read_enum(midi, "type", kMidiKinds, b.address.kind, error)) return false;
                b.address.channel = static_cast<std::uint8_t>(midi.value("channel", 0));
                b.address.number = static_cast<std::uint8_t>(midi.value("number", 0));
            }
            parsed.bindings.push_back(std::move(b));
        }
    }

    if (root.contains("mappings")) {
        if (!root["mappings"].is_array()) {
            error = "'mappings' must be an array";
            return false;
        }
        for (const json& node : root["mappings"]) {
            Mapping m;
            m.name = node.value("name", std::string());
            if (node.contains("enabled") && node["enabled"].is_boolean()) {
                m.enabled = node["enabled"].get<bool>();
            }

            if (node.contains("source") && node["source"].is_object()) {
                const json& source = node["source"];
                if (!read_enum(source, "kind", kSourceKinds, m.source.kind, error)) return false;
                m.source.control_id = source.value("control", std::string());
                m.source.deck = static_cast<std::uint8_t>(source.value("deck", 0));
                m.source.gesture_bit = source.value("gesture_bit", 0u);

                if (m.source.kind == SourceKind::Control && m.source.control_id.empty()) {
                    error = "mapping '" + m.name + "' has a control source with no 'control' id";
                    return false;
                }
            }

            if (node.contains("transform") && node["transform"].is_object()) {
                if (!transform_from_json(node["transform"], m.transform, error)) return false;
            }

            if (node.contains("destination") && node["destination"].is_object()) {
                const json& destination = node["destination"];
                if (!read_enum(destination, "kind", kDestinationKinds, m.destination.kind, error)) {
                    return false;
                }
                m.destination.target = destination.value("target", std::string());
            }

            parsed.mappings.push_back(std::move(m));
        }
    }

    out = std::move(parsed);
    return true;
}

bool config_save(const SurfaceConfig& config, const std::string& path, std::string& error) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "cannot open '" + path + "' for writing";
        return false;
    }
    file << config_to_json(config) << '\n';
    if (!file) {
        error = "failed while writing '" + path + "'";
        return false;
    }
    return true;
}

bool config_load(const std::string& path, SurfaceConfig& out, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open '" + path + "' for reading";
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();
    if (!config_from_json(text, out, error)) {
        error = path + ": " + error;
        return false;
    }
    return true;
}

void config_apply(const SurfaceConfig& config, Surface& surface) {
    for (const LearnResult& b : config.bindings) {
        const ControlIndex index = surface.declare(b.id, b.kind);
        surface.bind(b.address, index);
    }
}

}  // namespace svj
