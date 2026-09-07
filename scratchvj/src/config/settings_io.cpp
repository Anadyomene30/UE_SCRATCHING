#include "config/settings_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace svj {
namespace {

using json = nlohmann::json;

constexpr int kFormatVersion = 1;

}  // namespace

std::string settings_to_json(const DeskSettings& settings) {
    json root;
    root["version"] = kFormatVersion;
    root["platter"] = json{{"endpoint", settings.platter_endpoint},
                           {"first_channel", settings.platter_first_channel},
                           {"carrier_hz", settings.carrier_hz}};
    root["midi"] = json{{"port", settings.midi_port}};
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
