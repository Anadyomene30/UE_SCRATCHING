#include "config/settings_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace svj {
namespace {

using json = nlohmann::json;

constexpr int kFormatVersion = 1;

const char* curve_key(FaderCurve curve) {
    switch (curve) {
        case FaderCurve::Smooth: return "smooth";
        case FaderCurve::Linear: return "linear";
        case FaderCurve::Sharp: return "sharp";
        case FaderCurve::Cut: return "cut";
    }
    return "sharp";
}

bool curve_from_key(const std::string& key, FaderCurve& out) {
    for (const FaderCurve c : {FaderCurve::Smooth, FaderCurve::Linear, FaderCurve::Sharp,
                               FaderCurve::Cut}) {
        if (key == curve_key(c)) {
            out = c;
            return true;
        }
    }
    return false;
}

}  // namespace

std::string settings_to_json(const DeskSettings& settings) {
    json root;
    root["version"] = kFormatVersion;
    root["platter"] = json{{"endpoint", settings.platter_endpoint},
                           {"first_channel", settings.platter_first_channel},
                           {"carrier_hz", settings.carrier_hz}};
    json devices = json::array();
    for (const DeviceSetting& d : settings.devices) {
        devices.push_back(json{{"profile", d.profile},
                               {"port", d.port},
                               {"ordinal", d.ordinal},
                               {"deck", std::string(1, d.deck)}});
    }
    root["midi"] = json{{"port", settings.midi_port}, {"devices", devices}};
    root["output"] = json{{"display", settings.output_display}, {"open", settings.output_open}};
    root["engine"] = json{{"vram_budget_mb", settings.vram_budget_mb}};
    root["unreal"] = json{{"host", settings.control_host}, {"port", settings.control_port}};
    root["mix"] = json{{"xfader_curve", curve_key(settings.mix.xfader)},
                       {"fader_curve", curve_key(settings.mix.channel)},
                       {"fader_b_curve", curve_key(settings.mix.channel_b)},
                       {"xfader_reverse", settings.mix.xfader_reverse},
                       {"fader_reverse", settings.mix.channel_reverse},
                       {"fader_b_reverse", settings.mix.channel_b_reverse}};
    root["library"] = json{{"folders", settings.library_folders},
                           {"cache_dir", settings.cache_dir},
                           {"sequence_fps", settings.sequence_fps},
                           {"max_width", settings.analysis_max_width}};
    return root.dump(2);
}

bool settings_from_json(std::string_view text, DeskSettings& out, std::string& error) {
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

    // Built in a local and assigned only at the end, so a broken file cannot
    // half-apply -- a settings file that changed the endpoint but not the
    // channel pair would open the wrong input with confidence.
    DeskSettings parsed;

    if (root.contains("platter")) {
        const json& platter = root.at("platter");
        if (!platter.is_object()) {
            error = "'platter' doit etre un objet";
            return false;
        }
        if (platter.contains("endpoint")) {
            if (!platter.at("endpoint").is_string()) {
                error = "'platter.endpoint' doit etre une chaine";
                return false;
            }
            parsed.platter_endpoint = platter.at("endpoint").get<std::string>();
        }
        if (platter.contains("first_channel")) {
            if (!platter.at("first_channel").is_number_unsigned()) {
                error = "'platter.first_channel' doit etre un entier positif";
                return false;
            }
            parsed.platter_first_channel = platter.at("first_channel").get<unsigned>();
        }
        if (platter.contains("carrier_hz")) {
            if (!platter.at("carrier_hz").is_number() ||
                platter.at("carrier_hz").get<double>() <= 0.0) {
                error = "'platter.carrier_hz' doit etre un nombre positif";
                return false;
            }
            parsed.carrier_hz = platter.at("carrier_hz").get<double>();
        }
    }

    if (root.contains("midi")) {
        const json& midi = root.at("midi");
        if (!midi.is_object()) {
            error = "'midi' doit etre un objet";
            return false;
        }
        if (midi.contains("port")) {
            if (!midi.at("port").is_string()) {
                error = "'midi.port' doit etre une chaine";
                return false;
            }
            parsed.midi_port = midi.at("port").get<std::string>();
        }
        if (midi.contains("devices")) {
            if (!midi.at("devices").is_array()) {
                error = "'midi.devices' doit etre un tableau";
                return false;
            }
            std::vector<DeviceSetting> devices;
            for (const json& node : midi.at("devices")) {
                if (!node.is_object() || !node.contains("profile") ||
                    !node.at("profile").is_string()) {
                    error = "'midi.devices' : chaque appareil doit nommer son 'profile'";
                    return false;
                }
                DeviceSetting d;
                d.profile = node.at("profile").get<std::string>();
                d.port = node.value("port", std::string());
                d.ordinal = node.value("ordinal", 0);
                const std::string deck = node.value("deck", std::string("a"));
                d.deck = deck.empty() ? 'a' : deck[0];
                devices.push_back(std::move(d));
            }
            parsed.devices = std::move(devices);
        } else if (midi.contains("port")) {
            // A file from before the rig existed: one device, the mixer, on
            // the port it named. The turntables are not invented for it.
            parsed.devices = {DeviceSetting{"reloop_elite", parsed.midi_port, 0, 'a'}};
        }
    }

    if (root.contains("output")) {
        const json& output = root.at("output");
        if (!output.is_object()) {
            error = "'output' doit etre un objet";
            return false;
        }
        if (output.contains("display")) {
            if (!output.at("display").is_string()) {
                error = "'output.display' doit etre une chaine";
                return false;
            }
            parsed.output_display = output.at("display").get<std::string>();
        }
        parsed.output_open = output.value("open", false);
    }
    if (root.contains("engine")) {
        const json& engine = root.at("engine");
        if (!engine.is_object()) {
            error = "'engine' doit etre un objet";
            return false;
        }
        parsed.vram_budget_mb = engine.value("vram_budget_mb", parsed.vram_budget_mb);
        if (parsed.vram_budget_mb < 16) {
            error = "engine.vram_budget_mb doit valoir au moins 16";
            return false;
        }
    }
    if (root.contains("unreal")) {
        const json& unreal = root.at("unreal");
        if (!unreal.is_object()) {
            error = "'unreal' doit etre un objet";
            return false;
        }
        parsed.control_host = unreal.value("host", parsed.control_host);
        parsed.control_port = unreal.value("port", parsed.control_port);
        if (parsed.control_port == 0 || parsed.control_port > 65535) {
            error = "unreal.port doit etre entre 1 et 65535";
            return false;
        }
    }

    if (root.contains("mix")) {
        const json& mix = root.at("mix");
        if (!mix.is_object()) {
            error = "'mix' doit etre un objet";
            return false;
        }
        for (const char* key : {"xfader_curve", "fader_curve", "fader_b_curve"}) {
            if (!mix.contains(key)) continue;
            FaderCurve curve = FaderCurve::Sharp;
            if (!mix.at(key).is_string() || !curve_from_key(mix.at(key).get<std::string>(), curve)) {
                error = std::string("'mix.") + key + "' doit etre smooth, linear, sharp ou cut";
                return false;
            }
            const std::string name = key;
            (name == "xfader_curve" ? parsed.mix.xfader
             : name == "fader_curve" ? parsed.mix.channel
                                     : parsed.mix.channel_b) = curve;
        }
        // A file from before the second channel had its own: both the same.
        if (!mix.contains("fader_b_curve")) parsed.mix.channel_b = parsed.mix.channel;
        parsed.mix.xfader_reverse = mix.value("xfader_reverse", parsed.mix.xfader_reverse);
        parsed.mix.channel_reverse = mix.value("fader_reverse", parsed.mix.channel_reverse);
        parsed.mix.channel_b_reverse = mix.value("fader_b_reverse", parsed.mix.channel_reverse);
    }

    if (root.contains("library")) {
        const json& library = root.at("library");
        if (!library.is_object()) {
            error = "'library' doit etre un objet";
            return false;
        }
        if (library.contains("folders")) {
            if (!library.at("folders").is_array()) {
                error = "'library.folders' doit etre un tableau de chemins";
                return false;
            }
            std::vector<std::string> folders;
            for (const json& node : library.at("folders")) {
                if (!node.is_string()) {
                    error = "'library.folders' doit etre un tableau de chemins";
                    return false;
                }
                folders.push_back(node.get<std::string>());
            }
            parsed.library_folders = std::move(folders);
        }
        if (library.contains("cache_dir")) {
            if (!library.at("cache_dir").is_string()) {
                error = "'library.cache_dir' doit etre une chaine";
                return false;
            }
            parsed.cache_dir = library.at("cache_dir").get<std::string>();
        }
        if (library.contains("sequence_fps")) {
            if (!library.at("sequence_fps").is_number() ||
                library.at("sequence_fps").get<double>() <= 0.0) {
                error = "'library.sequence_fps' doit etre un nombre positif";
                return false;
            }
            parsed.sequence_fps = library.at("sequence_fps").get<double>();
        }
        if (library.contains("max_width")) {
            if (!library.at("max_width").is_number_unsigned() ||
                library.at("max_width").get<unsigned>() == 0) {
                error = "'library.max_width' doit etre un entier positif";
                return false;
            }
            parsed.analysis_max_width = library.at("max_width").get<unsigned>();
        }
    }

    out = parsed;
    return true;
}

bool settings_save(const DeskSettings& settings, const std::string& path,
                   std::string& error) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        error = "impossible d'ecrire " + path;
        return false;
    }
    file << settings_to_json(settings) << '\n';
    return true;
}

bool settings_load(const std::string& path, DeskSettings& out, std::string& error) {
    if (!std::filesystem::exists(path)) {
        // The normal first run on the desk the defaults were measured on.
        out = DeskSettings{};
        return true;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "impossible de lire " + path;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (!settings_from_json(buffer.str(), out, error)) {
        error = path + " : " + error;
        return false;
    }
    return true;
}

}  // namespace svj
