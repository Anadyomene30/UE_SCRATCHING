// scratchvj — finding the clips: folders in, library entries out.
//
// The library used to be a hard-coded glance at `clips/*.svcache`, and the
// three largest files went straight onto the decks. This replaces that with
// what a performer actually has: a few folders of rushes, some analysed and
// most not, and caches that should sit next to their source -- or, when the
// source lives on a read-only drive, in one cache folder of their own.
//
// In app/ rather than core/ because it walks the filesystem. Everything that
// can be decided without a disk -- which extensions count, where a cache goes,
// how a numbered sequence is recognised -- is a pure function here with a test.
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace svj {

// Case-insensitive, with or without the leading dot.
bool is_video_extension(std::string_view extension);
bool is_image_extension(std::string_view extension);

// Where the cache for `source` lives. Next to it (`source + ".svcache"`) when
// no cache folder is set; otherwise in the folder, named after the source's
// stem PLUS a hash of its full path, so two rushes both called intro.mp4 in
// different folders do not overwrite each other's cache.
std::string cache_path_for(const std::string& source, const std::string& cache_dir);

// A numbered image sequence, recognised from one of its files.
struct SequencePattern {
    std::string pattern;      // "C:/shots/frame_%04d.png", what ffmpeg reads
    std::string prefix;       // "C:/shots/frame_" -- what groups the files
    std::string extension;    // ".png"
    unsigned digits = 0;      // 4 for frame_0001
    unsigned number = 0;      // this file's number
};

// The pattern a file belongs to, from its NAME ALONE: a stem ending in digits.
// A file with no trailing digits is a still, not a sequence of one. Whether
// there really are other frames is the scan's business (see below).
std::optional<SequencePattern> sequence_of(const std::string& path);

// One thing the scan found.
struct ScanItem {
    std::string source;       // the video, still or sequence pattern; empty for an orphan cache
    std::string cache;        // where its .svcache is or will be
    bool cache_exists = false;
    bool is_still = false;
    bool is_sequence = false;
    unsigned sequence_start = 0;
};

// Groups image files into sequences and stills. PURE: takes paths, returns
// items, no disk. Two or more files sharing a prefix, a digit width and an
// extension are one sequence starting at the lowest number; a lone numbered
// file is a still, because "shot_01.png" alone is a picture, not a clip.
std::vector<ScanItem> group_images(const std::vector<std::string>& image_paths);

// Walks `folders` recursively. Videos become items; images are grouped; every
// .svcache is reported, with or without a source, so a cache whose rush has
// been moved is still playable and still listed.
std::vector<ScanItem> scan_folders(const std::vector<std::string>& folders,
                                   const std::string& cache_dir);

// The lowest frame number on disk for a sequence pattern: what the analysis
// pass has to start at. Zero when the pattern matches nothing. Sequence_of()
// on the pattern itself gives the pattern's own digits, not a real file's,
// which is why this reads the folder.
unsigned sequence_first_number(const std::string& pattern);

// The same, for files dropped on the window or picked in a dialog: a folder
// is scanned, a file is classified, a cache is taken as itself.
std::vector<ScanItem> scan_paths(const std::vector<std::string>& paths,
                                 const std::string& cache_dir);

}  // namespace svj
