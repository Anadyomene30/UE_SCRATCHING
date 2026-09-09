#include "config/profile_io.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace svj {
namespace {

using json = nlohmann::json;

constexpr int kFormatVersion = 1;

constexpr std::array<std::pair<ControlKind, const char*>, 5> kControlKinds{{
    {ControlKind::Fader, "fader"},
    {ControlKind::Knob, "knob"},
    {ControlKind::Button, "button"},
    {ControlKind::Encoder, "encoder"},
    {ControlKind::Pad, "pad"},
}};

constexpr std::array<std::pair<EncoderMode, const char*>, 3> kEncoderModes{{
    {EncoderMode::Absolute, "absolute"},
    {EncoderMode::Relative64, "relative64"},
    {EncoderMode::Signed7, "signed7"},
}};

constexpr std::array<std::pair<MidiKind, const char*>, 5> kMidiKinds{{
    {MidiKind::NoteOn, "note"},
    {MidiKind::NoteOff, "note_off"},
    {MidiKind::ControlChange, "cc"},
    {MidiKind::PitchBend, "pitchbend"},
    {MidiKind::Other, "other"},
}};

template <typename Enum, std::size_t N>
const char* name_of(const std::array<std::pair<Enum, const char*>, N>& table, Enum value) {
    for (const auto& entry : table) {
        if (entry.first == value) return entry.second;
    }
    return table[0].second;
}

template <typename Enum, std::size_t N>
bool read_enum(const json& node, const char* field,
               const std::array<std::pair<Enum, const char*>, N>& table, Enum& out,
               std::string& error) {
    if (!node.contains(field)) return true;
    if (!node.at(field).is_string()) {
        error = std::string("'") + field + "' doit etre une chaine";
        return false;
    }
    const auto text = node.at(field).get<std::string>();
    for (const auto& entry : table) {
        if (text == entry.second) {
            out = entry.first;
            return true;
        }
    }
    error = "valeur inconnue '" + text + "' pour '" + field + "'";
    return false;
}

}  // namespace

std::string profile_to_json(const DeviceProfile& profile) {
    json root;
    root["version"] = kFormatVersion;
    root["name"] = profile.name;
    root["display_name"] = profile.display_name;
    root["port_hint"] = profile.port_hint;
    root["verified"] = profile.verified;
    // Written only when there is one, so a profile without a note round
    // trips to the same JSON it came from.
    if (!profile.note.empty()) root["note"] = profile.note;
    if (profile.panel_w > 0.0f) {
        root["panel"] = json{{"width", profile.panel_w}, {"height", profile.panel_h}};
    }

    json controls = json::array();
    for (const ProfileControl& c : profile.controls) {
        json node{{"id", c.id}, {"kind", name_of(kControlKinds, c.kind)}};
        if (c.encoder != EncoderMode::Absolute) node["encoder"] = name_of(kEncoderModes, c.encoder);
        if (c.optional) node["optional"] = true;
        controls.push_back(node);
    }
    root["controls"] = controls;

    json layout = json::array();
    for (const ProfileGroup& g : profile.layout) {
        json node{{"title", g.title}, {"accent", g.accent}, {"controls", g.controls}};
        if (g.columns > 0) node["columns"] = g.columns;
        if (g.placed()) {
            node["at"] = json{{"x", g.x}, {"y", g.y}, {"w", g.w}, {"h", g.h}};
        }
        if (g.front_panel) node["front_panel"] = true;
        layout.push_back(node);
    }
    root["layout"] = layout;

    json bindings = json::array();
    for (const ProfileBinding& b : profile.bindings) {
        bindings.push_back(json{{"id", b.id},
                                {"midi", json{{"type", name_of(kMidiKinds, b.address.kind)},
                                              {"channel", b.address.channel},
                                              {"number", b.address.number}}}});
    }
    root["bindings"] = bindings;
    return root.dump(2);
}

bool profile_from_json(std::string_view text, DeviceProfile& out, std::string& error) {
    json root;
    try {
        root = json::parse(text);
    } catch (const json::exception& e) {
        error = std::string("JSON invalide : ") + e.what();
        return false;
    }
    if (!root.is_object()) {
        error = "la racine doit etre un objet";
        return false;
    }

    DeviceProfile parsed;
    if (!root.contains("name") || !root.at("name").is_string()) {
        error = "'name' est obligatoire";
        return false;
    }
    parsed.name = root.at("name").get<std::string>();
    parsed.display_name = root.value("display_name", parsed.name);
    parsed.port_hint = root.value("port_hint", std::string());
    parsed.verified = root.value("verified", false);
    parsed.note = root.value("note", std::string{});
    if (root.contains("panel") && root.at("panel").is_object()) {
        parsed.panel_w = root.at("panel").value("width", 0.0f);
        parsed.panel_h = root.at("panel").value("height", 0.0f);
    }

    if (root.contains("controls")) {
        if (!root.at("controls").is_array()) {
            error = "'controls' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("controls")) {
            if (!node.is_object() || !node.contains("id") || !node.at("id").is_string()) {
                error = "chaque contrôle doit avoir un 'id'";
                return false;
            }
            ProfileControl c;
            c.id = node.at("id").get<std::string>();
            if (!read_enum(node, "kind", kControlKinds, c.kind, error)) return false;
            if (!read_enum(node, "encoder", kEncoderModes, c.encoder, error)) return false;
            c.optional = node.value("optional", false);
            parsed.controls.push_back(std::move(c));
        }
    }

    if (root.contains("layout")) {
        if (!root.at("layout").is_array()) {
            error = "'layout' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("layout")) {
            ProfileGroup g;
            g.title = node.value("title", std::string());
            g.accent = node.value("accent", std::string("ink"));
            g.columns = node.value("columns", 0);
            g.front_panel = node.value("front_panel", false);
            if (node.contains("at") && node.at("at").is_object()) {
                const json& at = node.at("at");
                g.x = at.value("x", 0.0f);
                g.y = at.value("y", 0.0f);
                g.w = at.value("w", 0.0f);
                g.h = at.value("h", 0.0f);
            }
            for (const json& id : node.value("controls", json::array())) {
                if (!id.is_string()) {
                    error = "layout.controls doit contenir des ids";
                    return false;
                }
                g.controls.push_back(id.get<std::string>());
            }
            parsed.layout.push_back(std::move(g));
        }
    }

    if (root.contains("bindings")) {
        if (!root.at("bindings").is_array()) {
            error = "'bindings' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("bindings")) {
            if (!node.is_object() || !node.contains("id") || !node.at("id").is_string()) {
                error = "chaque liaison doit avoir un 'id'";
                return false;
            }
            ProfileBinding b;
            b.id = node.at("id").get<std::string>();
            if (node.contains("midi") && node.at("midi").is_object()) {
                const json& midi = node.at("midi");
                if (!read_enum(midi, "type", kMidiKinds, b.address.kind, error)) return false;
                b.address.channel = static_cast<std::uint8_t>(midi.value("channel", 0));
                b.address.number = static_cast<std::uint8_t>(midi.value("number", 0));
            }
            parsed.bindings.push_back(std::move(b));
        }
    }

    const std::vector<std::string> faults = validate_profile(parsed);
    if (!faults.empty()) {
        error = faults.front();
        return false;
    }
    out = std::move(parsed);
    return true;
}

bool profile_save(const DeviceProfile& profile, const std::string& path, std::string& error) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible d'ecrire " + path;
        return false;
    }
    stream << profile_to_json(profile) << '\n';
    return true;
}

bool profile_load(const std::string& path, DeviceProfile& out, std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible de lire " + path;
        return false;
    }
    std::stringstream buffer;
    buffer << stream.rdbuf();
    if (!profile_from_json(buffer.str(), out, error)) {
        error = path + " : " + error;
        return false;
    }
    return true;
}

std::vector<DeviceProfile> profiles_in(const std::string& directory,
                                       std::vector<std::string>& errors) {
    std::vector<DeviceProfile> loaded;
    std::error_code missing;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory, missing)) {
        if (entry.path().extension() == ".json") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const auto& file : files) {
        DeviceProfile profile;
        std::string error;
        if (profile_load(file.string(), profile, error)) {
            loaded.push_back(std::move(profile));
        } else {
            errors.push_back(error);
        }
    }
    return loaded;
}

const DeviceProfile* find_profile(std::string_view name,
                                  const std::vector<DeviceProfile>& loaded) {
    for (const DeviceProfile& p : loaded) {
        if (p.name == name) return &p;
    }
    return builtin_profile(name);
}

}  // namespace svj
