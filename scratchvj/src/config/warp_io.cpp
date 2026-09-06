#include "config/warp_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace svj {
namespace {

using json = nlohmann::json;

constexpr const char* kExtension = ".svmap";
constexpr int kFormatVersion = 1;

json point_to_json(const Point& p) { return json{{"x", p.x}, {"y", p.y}}; }

bool point_from_json(const json& node, Point& out, std::string& error) {
    if (!node.is_object() || !node.contains("x") || !node.contains("y")) {
        error = "un point doit avoir x et y";
        return false;
    }
    out.x = node.at("x").get<double>();
    out.y = node.at("y").get<double>();
    return true;
}

}  // namespace

std::string preset_to_json(const OutputPreset& preset) {
    json root;
    root["version"] = kFormatVersion;
    root["name"] = preset.name;

    root["pin"] = json{{"top_left", point_to_json(preset.pin.top_left)},
                       {"top_right", point_to_json(preset.pin.top_right)},
                       {"bottom_right", point_to_json(preset.pin.bottom_right)},
                       {"bottom_left", point_to_json(preset.pin.bottom_left)}};

    json mesh;
    mesh["enabled"] = preset.mesh_enabled;
    mesh["cols"] = preset.mesh.cols();
    mesh["rows"] = preset.mesh.rows();
    mesh["interpolation"] =
        preset.mesh.interpolation() == WarpInterpolation::Bezier ? "bezier" : "bilinear";
    // The source coordinates travel with the points: they stopped being evenly
    // spaced the first time a line was inserted, and a preset that reconstructed
    // them by division would redistribute the whole picture on load.
    json columns = json::array();
    for (int col = 0; col < preset.mesh.cols(); ++col) {
        columns.push_back(preset.mesh.column_u(col));
    }
    json rows = json::array();
    for (int row = 0; row < preset.mesh.rows(); ++row) {
        rows.push_back(preset.mesh.row_v(row));
    }
    mesh["source_u"] = columns;
    mesh["source_v"] = rows;

    json points = json::array();
    for (int row = 0; row < preset.mesh.rows(); ++row) {
        for (int col = 0; col < preset.mesh.cols(); ++col) {
            points.push_back(point_to_json(preset.mesh.at(col, row)));
        }
    }
    mesh["points"] = points;
    root["mesh"] = mesh;

    json mask = json::array();
    for (const Point& p : preset.mask) mask.push_back(point_to_json(p));
    root["mask"] = json{{"feather", preset.mask_feather}, {"polygon", mask}};

    return root.dump(2);
}

bool preset_from_json(std::string_view json_text, OutputPreset& out, std::string& error) {
    json root = json::parse(json_text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        error = "JSON illisible";
        return false;
    }
    if (root.value("version", 0) != kFormatVersion) {
        error = "version de preset inconnue";
        return false;
    }

    OutputPreset parsed;
    parsed.name = root.value("name", std::string{});

    if (root.contains("pin")) {
        const json& pin = root.at("pin");
        if (!point_from_json(pin.value("top_left", json{}), parsed.pin.top_left, error) ||
            !point_from_json(pin.value("top_right", json{}), parsed.pin.top_right, error) ||
            !point_from_json(pin.value("bottom_right", json{}), parsed.pin.bottom_right,
                             error) ||
            !point_from_json(pin.value("bottom_left", json{}), parsed.pin.bottom_left,
                             error)) {
            error = "pin: " + error;
            return false;
        }
    }

    if (root.contains("mesh")) {
        const json& mesh = root.at("mesh");
        const int cols = mesh.value("cols", 2);
        const int rows = mesh.value("rows", 2);
        if (cols < 2 || rows < 2 || cols > kMaxWarpDivisions || rows > kMaxWarpDivisions) {
            error = "mesh: dimensions hors limites";
            return false;
        }
        const json& points = mesh.value("points", json::array());
        if (!points.is_array() ||
            points.size() != static_cast<std::size_t>(cols) * rows) {
            error = "mesh: le nombre de points ne correspond pas a la grille";
            return false;
        }

        // Size first, then the source spacing the file recorded. Replaying
        // insertions instead would rebuild a grid of the right SIZE with the
        // wrong spacing -- every corner in place and the picture between them
        // slid, which is the kind of fault nobody spots until the projector is
        // on the wall.
        parsed.mesh.reset(cols, rows);
        if (mesh.contains("source_u") && mesh.contains("source_v")) {
            const std::vector<double> source_u =
                mesh.at("source_u").get<std::vector<double>>();
            const std::vector<double> source_v =
                mesh.at("source_v").get<std::vector<double>>();
            if (!parsed.mesh.set_source_coordinates(source_u, source_v)) {
                error = "mesh: coordonnees source invalides (doivent monter de 0 a 1)";
                return false;
            }
        }

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                Point p;
                if (!point_from_json(points.at(static_cast<std::size_t>(row) * cols + col), p,
                                     error)) {
                    error = "mesh: " + error;
                    return false;
                }
                parsed.mesh.set(col, row, p);
            }
        }
        parsed.mesh.set_interpolation(mesh.value("interpolation", std::string{"bilinear"}) ==
                                              "bezier"
                                          ? WarpInterpolation::Bezier
                                          : WarpInterpolation::Bilinear);
        parsed.mesh_enabled = mesh.value("enabled", false);
    }

    if (root.contains("mask")) {
        const json& mask = root.at("mask");
        parsed.mask_feather = mask.value("feather", 0.0);
        for (const json& node : mask.value("polygon", json::array())) {
            Point p;
            if (!point_from_json(node, p, error)) {
                error = "mask: " + error;
                return false;
            }
            parsed.mask.push_back(p);
        }
    }

    out = std::move(parsed);
    return true;
}

bool preset_save(const OutputPreset& preset, const std::string& path, std::string& error) {
    std::error_code missing;
    const std::filesystem::path file(path);
    if (file.has_parent_path()) {
        std::filesystem::create_directories(file.parent_path(), missing);
    }
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible d'ecrire " + path;
        return false;
    }
    stream << preset_to_json(preset);
    if (!stream) {
        error = "ecriture interrompue: " + path;
        return false;
    }
    return true;
}

bool preset_load(const std::string& path, OutputPreset& out, std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible de lire " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return preset_from_json(buffer.str(), out, error);
}

std::string preset_path(const std::string& directory, const std::string& name) {
    return (std::filesystem::path(directory) / (name + kExtension)).string();
}

std::vector<std::string> preset_names(const std::string& directory) {
    std::vector<std::string> names;
    std::error_code missing;
    for (const auto& entry : std::filesystem::directory_iterator(directory, missing)) {
        if (entry.path().extension() == kExtension) {
            names.push_back(entry.path().stem().string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace svj
