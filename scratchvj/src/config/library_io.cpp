#include "config/library_io.h"

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

constexpr std::array<std::pair<ProjectionOverride, const char*>, 3> kProjections{{
    {ProjectionOverride::Auto, "auto"},
    {ProjectionOverride::Flat, "flat"},
    {ProjectionOverride::Equirect, "equirect"},
}};

const char* projection_name(ProjectionOverride p) {
    for (const auto& entry : kProjections) {
        if (entry.first == p) return entry.second;
    }
    return kProjections[0].second;
}

bool projection_value(std::string_view name, ProjectionOverride& out) {
    for (const auto& entry : kProjections) {
        if (name == entry.second) {
            out = entry.first;
            return true;
        }
    }
    return false;
}

bool read_string(const json& node, const char* field, std::string& out, std::string& error,
                 const char* where) {
    if (!node.contains(field)) return true;
    if (!node.at(field).is_string()) {
        error = std::string(where) + "." + field + " doit etre une chaine";
        return false;
    }
    out = node.at(field).get<std::string>();
    return true;
}

}  // namespace

std::string library_to_json(const LibraryFile& file) {
    json root;
    root["version"] = kFormatVersion;
    json clips = json::array();
    for (const LibraryFile::Clip& clip : file.clips) {
        json node;
        node["source"] = clip.source;
        node["cache"] = clip.cache;
        node["name"] = clip.name;
        node["projection"] = projection_name(clip.projection);
        if (clip.is_sequence) node["sequence"] = true;
        if (clip.is_still) node["still"] = true;
        clips.push_back(node);
    }
    root["clips"] = clips;
    json crates = json::array();
    for (const LibraryFile::Crate& crate : file.crates) {
        crates.push_back(json{{"name", crate.name}, {"clips", crate.clips}});
    }
    root["crates"] = crates;
    if (!file.banks.empty()) {
        json banks = json::array();
        for (const LibraryFile::Bank& bank : file.banks) {
            banks.push_back(json{{"name", bank.name},
                                 {"a", bank.a},
                                 {"b", bank.b},
                                 {"overlay", bank.overlay}});
        }
        root["banks"] = banks;
        root["current_bank"] = file.current_bank;
    }
    return root.dump(2);
}

bool library_from_json(std::string_view text, LibraryFile& out, std::string& error) {
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

    LibraryFile parsed;
    if (root.contains("clips")) {
        if (!root.at("clips").is_array()) {
            error = "'clips' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("clips")) {
            if (!node.is_object()) {
                error = "chaque entree de 'clips' doit etre un objet";
                return false;
            }
            LibraryFile::Clip clip;
            if (!read_string(node, "source", clip.source, error, "clips")) return false;
            if (!read_string(node, "cache", clip.cache, error, "clips")) return false;
            if (!read_string(node, "name", clip.name, error, "clips")) return false;
            if (clip.cache.empty()) {
                error = "clips.cache est obligatoire";
                return false;
            }
            if (node.contains("projection")) {
                if (!node.at("projection").is_string() ||
                    !projection_value(node.at("projection").get<std::string>(),
                                      clip.projection)) {
                    error = "clips.projection inconnue : " + node.at("projection").dump();
                    return false;
                }
            }
            clip.is_sequence = node.value("sequence", false);
            clip.is_still = node.value("still", false);
            parsed.clips.push_back(std::move(clip));
        }
    }
    if (root.contains("crates")) {
        if (!root.at("crates").is_array()) {
            error = "'crates' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("crates")) {
            LibraryFile::Crate crate;
            if (!node.is_object() || !read_string(node, "name", crate.name, error, "crates")) {
                if (error.empty()) error = "chaque entree de 'crates' doit etre un objet";
                return false;
            }
            for (const json& member : node.value("clips", json::array())) {
                if (!member.is_string()) {
                    error = "crates.clips doit contenir des chemins";
                    return false;
                }
                crate.clips.push_back(member.get<std::string>());
            }
            parsed.crates.push_back(std::move(crate));
        }
    }

    if (root.contains("banks")) {
        if (!root.at("banks").is_array()) {
            error = "'banks' doit etre un tableau";
            return false;
        }
        for (const json& node : root.at("banks")) {
            LibraryFile::Bank bank;
            if (!node.is_object() || !read_string(node, "name", bank.name, error, "banks")) {
                if (error.empty()) error = "chaque entree de 'banks' doit etre un objet";
                return false;
            }
            const auto read_cells = [&](const char* field, std::vector<std::string>& cells) {
                for (const json& cell : node.value(field, json::array())) {
                    if (!cell.is_string()) {
                        error = std::string("banks.") + field + " doit contenir des chemins";
                        return false;
                    }
                    cells.push_back(cell.get<std::string>());
                }
                return true;
            };
            if (!read_cells("a", bank.a) || !read_cells("b", bank.b) ||
                !read_cells("overlay", bank.overlay)) {
                return false;
            }
            parsed.banks.push_back(std::move(bank));
        }
        parsed.current_bank = root.value("current_bank", 0);
    }

    out = std::move(parsed);
    return true;
}

bool library_save(const LibraryFile& file, const std::string& path, std::string& error) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible d'ecrire " + path;
        return false;
    }
    stream << library_to_json(file) << '\n';
    return true;
}

bool library_load(const std::string& path, LibraryFile& out, std::string& error) {
    if (!std::filesystem::exists(path)) {
        out = LibraryFile{};
        return true;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "impossible de lire " + path;
        return false;
    }
    std::stringstream buffer;
    buffer << stream.rdbuf();
    if (!library_from_json(buffer.str(), out, error)) {
        error = path + " : " + error;
        return false;
    }
    return true;
}

LibraryFile library_snapshot(const Library& library) {
    LibraryFile file;
    for (std::size_t i = 0; i < library.size(); ++i) {
        const ClipEntry& entry = library.at(static_cast<ClipId>(i));
        LibraryFile::Clip clip;
        clip.source = entry.source_path;
        clip.cache = entry.path;
        clip.name = entry.name;
        clip.projection = entry.projection;
        clip.is_sequence = entry.is_sequence;
        clip.is_still = entry.is_still;
        file.clips.push_back(std::move(clip));
    }
    for (int c = 0; c < library.crate_count(); ++c) {
        LibraryFile::Crate crate;
        crate.name = library.crate_name(c);
        for (const ClipId id : library.crate_clips(c)) crate.clips.push_back(library.at(id).path);
        file.crates.push_back(std::move(crate));
    }
    return file;
}

LibraryFile library_snapshot(const Library& library, const Matrix& matrix) {
    LibraryFile file = library_snapshot(library);
    for (int b = 0; b < matrix.bank_count(); ++b) {
        const PadBank& bank = matrix.bank(b);
        LibraryFile::Bank out;
        out.name = bank.name;
        const auto paths = [&](const std::array<ClipId, kPadCount>& cells,
                               std::vector<std::string>& into) {
            for (const ClipId id : cells) {
                into.push_back(id == kNoClip || static_cast<std::size_t>(id) >= library.size()
                                   ? std::string()
                                   : library.at(id).path);
            }
        };
        paths(bank.a, out.a);
        paths(bank.b, out.b);
        paths(bank.overlay, out.overlay);
        file.banks.push_back(std::move(out));
    }
    file.current_bank = matrix.current();
    return file;
}

void library_restore(const LibraryFile& file, Library& library, Matrix& matrix) {
    library_restore(file, library);
    if (file.banks.empty()) return;
    // Rebuilt from scratch: the file is the memory, the matrix the reading.
    matrix = Matrix{};
    for (std::size_t b = 0; b < file.banks.size(); ++b) {
        const LibraryFile::Bank& bank = file.banks[b];
        const int index = b == 0 ? 0 : matrix.add_bank(bank.name);
        if (b == 0) matrix.mutable_bank(0).name = bank.name;
        const auto cells = [&](const std::vector<std::string>& paths, DeckTarget target) {
            for (std::size_t pad = 0; pad < paths.size() && pad < static_cast<std::size_t>(kPadCount); ++pad) {
                if (paths[pad].empty()) continue;
                const ClipId id = library.find_by_path(paths[pad]);
                if (id != kNoClip) matrix.set(index, target, static_cast<int>(pad), id);
            }
        };
        cells(bank.a, DeckTarget::A);
        cells(bank.b, DeckTarget::B);
        cells(bank.overlay, DeckTarget::Overlay);
    }
    matrix.select(file.current_bank);
}

void library_restore(const LibraryFile& file, Library& library) {
    for (const LibraryFile::Clip& clip : file.clips) {
        if (library.find_by_path(clip.cache) != kNoClip) continue;
        ClipEntry entry;
        entry.path = clip.cache;
        entry.source_path = clip.source;
        entry.name = clip.name;
        entry.projection = clip.projection;
        entry.is_sequence = clip.is_sequence;
        entry.is_still = clip.is_still;
        entry.state = AnalysisState::Unanalysed;
        library.add(entry);
    }
    for (const LibraryFile::Crate& crate : file.crates) {
        const int index = library.create_crate(crate.name);
        for (const std::string& cache : crate.clips) {
            const ClipId id = library.find_by_path(cache);
            if (id != kNoClip) library.add_to_crate(index, id);
        }
    }
}

}  // namespace svj
