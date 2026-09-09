#include "app/library_scan.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <map>

namespace svj {
namespace {

namespace fs = std::filesystem;

std::string lowered(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string bare_extension(std::string_view extension) {
    std::string ext = lowered(extension);
    if (!ext.empty() && ext.front() == '.') ext.erase(0, 1);
    return ext;
}

// What ffmpeg opens without help and what a VJ actually carries. Not a claim
// about what ffmpeg CAN decode -- it decodes far more -- but about what to
// pick up when walking a folder of mixed files without asking.
constexpr const char* kVideoExtensions[] = {"mp4", "m4v", "mov", "mkv", "webm", "avi",
                                            "mxf", "ts",  "mts", "m2ts", "mpg",  "mpeg",
                                            "wmv", "flv", "ogv", "gif"};
constexpr const char* kImageExtensions[] = {"png", "jpg", "jpeg", "tif", "tiff",
                                            "bmp", "exr", "dpx", "tga",  "webp"};

std::uint32_t fnv1a(std::string_view text) {
    std::uint32_t hash = 2166136261u;
    for (const unsigned char c : text) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

// Forward slashes throughout, so the same path hashes the same way whether it
// came from a dialog, a drop or a settings file typed by hand.
std::string normalised(const std::string& path) {
    std::string out = path;
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

ScanItem video_item(const std::string& source, const std::string& cache_dir) {
    ScanItem item;
    item.source = normalised(source);
    item.cache = cache_path_for(item.source, cache_dir);
    std::error_code ignored;
    item.cache_exists = fs::exists(item.cache, ignored);
    return item;
}

void collect(const fs::path& folder, std::vector<std::string>& videos,
             std::vector<std::string>& images, std::vector<std::string>& caches) {
    std::error_code missing;
    fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied,
                                        missing);
    const fs::recursive_directory_iterator end;
    while (!missing && it != end) {
        const fs::directory_entry& entry = *it;
        if (entry.is_regular_file(missing)) {
            const std::string ext = bare_extension(entry.path().extension().string());
            const std::string path = normalised(entry.path().string());
            if (ext == "svcache") {
                caches.push_back(path);
            } else if (is_video_extension(ext)) {
                videos.push_back(path);
            } else if (is_image_extension(ext)) {
                images.push_back(path);
            }
        }
        it.increment(missing);
    }
}

}  // namespace

bool is_video_extension(std::string_view extension) {
    const std::string ext = bare_extension(extension);
    for (const char* known : kVideoExtensions) {
        if (ext == known) return true;
    }
    return false;
}

bool is_image_extension(std::string_view extension) {
    const std::string ext = bare_extension(extension);
    for (const char* known : kImageExtensions) {
        if (ext == known) return true;
    }
    return false;
}

std::string cache_path_for(const std::string& source, const std::string& cache_dir) {
    std::string src = normalised(source);
    // A sequence pattern names its cache "frame_seq.png.svcache": a percent
    // sign in a file name is legal but reads as a mistake in every file
    // browser, and ffmpeg would take it for a pattern again.
    const std::size_t percent = src.find("%0");
    if (percent != std::string::npos) {
        const std::size_t d = src.find('d', percent);
        if (d != std::string::npos) src.replace(percent, d - percent + 1, "seq");
    }
    if (cache_dir.empty()) return src + ".svcache";
    const fs::path source_path(src);
    char hash[16];
    std::snprintf(hash, sizeof(hash), "%08x", fnv1a(src));
    const std::string name = source_path.stem().string() + "-" + hash + ".svcache";
    return normalised((fs::path(cache_dir) / name).string());
}

std::optional<SequencePattern> sequence_of(const std::string& path) {
    const std::string src = normalised(path);
    const fs::path p(src);
    const std::string stem = p.stem().string();
    std::size_t digits = 0;
    while (digits < stem.size() && std::isdigit(static_cast<unsigned char>(stem[stem.size() - 1 - digits]))) {
        ++digits;
    }
    if (digits == 0) return std::nullopt;

    SequencePattern out;
    out.digits = static_cast<unsigned>(digits);
    out.extension = p.extension().string();
    const std::string head = stem.substr(0, stem.size() - digits);
    const std::string parent = p.has_parent_path() ? normalised(p.parent_path().string()) + "/" : "";
    out.prefix = parent + head;
    out.number = static_cast<unsigned>(std::stoul(stem.substr(stem.size() - digits)));
    out.pattern = out.prefix + "%0" + std::to_string(digits) + "d" + out.extension;
    return out;
}

std::vector<ScanItem> group_images(const std::vector<std::string>& image_paths) {
    struct Group {
        SequencePattern pattern;
        std::vector<unsigned> numbers;
        std::string first_file;
    };
    // Keyed on what makes files one sequence: prefix, width and extension.
    std::map<std::string, Group> groups;
    std::vector<ScanItem> stills;

    for (const std::string& path : image_paths) {
        const auto seq = sequence_of(path);
        if (!seq) {
            ScanItem item;
            item.source = normalised(path);
            item.is_still = true;
            stills.push_back(item);
            continue;
        }
        const std::string key = seq->prefix + "|" + std::to_string(seq->digits) + "|" +
                                lowered(seq->extension);
        Group& group = groups[key];
        if (group.numbers.empty()) {
            group.pattern = *seq;
            group.first_file = normalised(path);
        }
        group.numbers.push_back(seq->number);
    }

    std::vector<ScanItem> items;
    for (auto& [key, group] : groups) {
        if (group.numbers.size() < 2) {
            // One numbered file is a picture, not a one-frame clip.
            ScanItem item;
            item.source = group.first_file;
            item.is_still = true;
            items.push_back(item);
            continue;
        }
        ScanItem item;
        item.source = group.pattern.pattern;
        item.is_sequence = true;
        item.sequence_start = *std::min_element(group.numbers.begin(), group.numbers.end());
        items.push_back(item);
    }
    items.insert(items.end(), stills.begin(), stills.end());
    std::sort(items.begin(), items.end(),
              [](const ScanItem& a, const ScanItem& b) { return a.source < b.source; });
    return items;
}

std::vector<ScanItem> scan_folders(const std::vector<std::string>& folders,
                                   const std::string& cache_dir) {
    std::vector<std::string> videos, images, caches;
    for (const std::string& folder : folders) collect(fs::path(folder), videos, images, caches);

    std::vector<ScanItem> items;
    std::vector<std::string> claimed_caches;
    for (const std::string& video : videos) {
        items.push_back(video_item(video, cache_dir));
        claimed_caches.push_back(items.back().cache);
    }
    for (ScanItem item : group_images(images)) {
        item.cache = cache_path_for(item.source, cache_dir);
        std::error_code ignored;
        item.cache_exists = fs::exists(item.cache, ignored);
        claimed_caches.push_back(item.cache);
        items.push_back(item);
    }
    // Caches nobody above claimed: orphans, or caches of sources kept
    // elsewhere. Listed as playable, with no source to re-analyse.
    for (const std::string& cache : caches) {
        if (std::find(claimed_caches.begin(), claimed_caches.end(), cache) !=
            claimed_caches.end()) {
            continue;
        }
        ScanItem item;
        item.cache = cache;
        item.cache_exists = true;
        items.push_back(item);
    }
    std::sort(items.begin(), items.end(), [](const ScanItem& a, const ScanItem& b) {
        const std::string& ka = a.source.empty() ? a.cache : a.source;
        const std::string& kb = b.source.empty() ? b.cache : b.source;
        return ka < kb;
    });
    return items;
}

unsigned sequence_first_number(const std::string& pattern) {
    const auto seq = sequence_of(pattern);
    if (!seq) return 0;
    const fs::path parent = fs::path(pattern).parent_path();
    std::error_code ignored;
    bool found = false;
    unsigned lowest = 0;
    for (const auto& entry : fs::directory_iterator(parent, ignored)) {
        if (!entry.is_regular_file(ignored)) continue;
        const auto other = sequence_of(normalised(entry.path().string()));
        if (!other || other->prefix != seq->prefix || other->digits != seq->digits ||
            lowered(other->extension) != lowered(seq->extension)) {
            continue;
        }
        if (!found || other->number < lowest) lowest = other->number;
        found = true;
    }
    return lowest;
}

std::vector<ScanItem> scan_paths(const std::vector<std::string>& paths,
                                 const std::string& cache_dir) {
    std::vector<ScanItem> items;
    std::vector<std::string> folders, images;
    std::error_code ignored;
    for (const std::string& raw : paths) {
        const std::string path = normalised(raw);
        if (fs::is_directory(path, ignored)) {
            folders.push_back(path);
            continue;
        }
        const std::string ext = bare_extension(fs::path(path).extension().string());
        if (ext == "svcache") {
            ScanItem item;
            item.cache = path;
            item.cache_exists = fs::exists(path, ignored);
            items.push_back(item);
        } else if (is_video_extension(ext)) {
            items.push_back(video_item(path, cache_dir));
        } else if (is_image_extension(ext)) {
            images.push_back(path);
        }
    }
    // A dropped frame of a sequence brings its siblings: the performer meant
    // the shot, not one picture of it.
    std::vector<std::string> expanded;
    for (const std::string& image : images) {
        const auto seq = sequence_of(image);
        if (!seq) {
            expanded.push_back(image);
            continue;
        }
        const fs::path parent = fs::path(image).parent_path();
        bool found_sibling = false;
        for (const auto& entry : fs::directory_iterator(parent, ignored)) {
            if (!entry.is_regular_file(ignored)) continue;
            const auto other = sequence_of(normalised(entry.path().string()));
            if (other && other->prefix == seq->prefix && other->digits == seq->digits &&
                lowered(other->extension) == lowered(seq->extension)) {
                expanded.push_back(normalised(entry.path().string()));
                if (entry.path() != fs::path(image)) found_sibling = true;
            }
        }
        if (!found_sibling) expanded.push_back(image);
    }
    std::sort(expanded.begin(), expanded.end());
    expanded.erase(std::unique(expanded.begin(), expanded.end()), expanded.end());
    for (ScanItem item : group_images(expanded)) {
        item.cache = cache_path_for(item.source, cache_dir);
        item.cache_exists = fs::exists(item.cache, ignored);
        items.push_back(item);
    }
    if (!folders.empty()) {
        const std::vector<ScanItem> inside = scan_folders(folders, cache_dir);
        items.insert(items.end(), inside.begin(), inside.end());
    }
    return items;
}

}  // namespace svj
