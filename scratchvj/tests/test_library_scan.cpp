#include <string>
#include <vector>

#include "app/library_scan.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("library_scan: the cache sits next to its source when no cache folder is set") {
    CHECK_EQ(cache_path_for("D:/rushes/intro.mp4", ""),
             std::string("D:/rushes/intro.mp4.svcache"));
    // Backslashes are normalised, so a path from a dialog and the same path
    // typed into settings.json name the same cache.
    CHECK_EQ(cache_path_for("D:\\rushes\\intro.mp4", ""),
             std::string("D:/rushes/intro.mp4.svcache"));
}

SVJ_TEST("library_scan: a cache folder names caches by stem and path hash") {
    const std::string cache = cache_path_for("D:/rushes/intro.mp4", "E:/svcache");
    CHECK(cache.rfind("E:/svcache/intro-", 0) == 0);
    CHECK(cache.size() > std::string("E:/svcache/intro-.svcache").size());
    CHECK(cache.substr(cache.size() - 8) == ".svcache");
}

SVJ_TEST("library_scan: two sources with the same stem get different caches") {
    // The whole reason for the hash: a set folder and a loops folder both
    // holding intro.mp4 must not fight over one cache file.
    const std::string a = cache_path_for("D:/set/intro.mp4", "E:/svcache");
    const std::string b = cache_path_for("D:/loops/intro.mp4", "E:/svcache");
    CHECK(a != b);
    // And the same source always maps to the same cache.
    CHECK_EQ(a, cache_path_for("D:/set/intro.mp4", "E:/svcache"));
}

SVJ_TEST("library_scan: the extension tables know the classics and ignore the rest") {
    CHECK(is_video_extension(".mp4"));
    CHECK(is_video_extension("MOV"));
    CHECK(is_video_extension(".mkv"));
    CHECK(is_video_extension("webm"));
    CHECK(!is_video_extension(".svcache"));
    CHECK(!is_video_extension(".txt"));
    CHECK(is_image_extension(".png"));
    CHECK(is_image_extension("JPG"));
    CHECK(is_image_extension(".exr"));
    CHECK(!is_image_extension(".mp4"));
}

SVJ_TEST("library_scan: a numbered frame names its sequence pattern") {
    const auto seq = sequence_of("D:/shots/frame_0001.png");
    CHECK(seq.has_value());
    CHECK_EQ(seq->pattern, std::string("D:/shots/frame_%04d.png"));
    CHECK_EQ(seq->prefix, std::string("D:/shots/frame_"));
    CHECK_EQ(seq->digits, 4u);
    CHECK_EQ(seq->number, 1u);
    CHECK_EQ(seq->extension, std::string(".png"));
}

SVJ_TEST("library_scan: a file with no trailing digits is not a sequence") {
    CHECK(!sequence_of("D:/shots/logo.png").has_value());
    CHECK(!sequence_of("logo_final.tif").has_value());
}

SVJ_TEST("library_scan: grouping turns siblings into one sequence and leaves a lone frame a still") {
    // shot_01.png alone is a picture, not a one-frame clip; three frames of
    // the same prefix and width are one clip starting at the lowest number.
    const std::vector<std::string> files = {
        "D:/a/frame_0003.png", "D:/a/frame_0001.png", "D:/a/frame_0002.png",
        "D:/a/shot_01.png",    "D:/a/logo.png",
    };
    const auto items = group_images(files);
    CHECK_EQ(items.size(), std::size_t{3});
    int sequences = 0, stills = 0;
    for (const ScanItem& item : items) {
        if (item.is_sequence) {
            ++sequences;
            CHECK_EQ(item.source, std::string("D:/a/frame_%04d.png"));
            CHECK_EQ(item.sequence_start, 1u);
        } else {
            ++stills;
            CHECK(item.is_still);
        }
    }
    CHECK_EQ(sequences, 1);
    CHECK_EQ(stills, 2);
}

SVJ_TEST("library_scan: frames of different widths or extensions are different sequences") {
    const std::vector<std::string> files = {"x_001.png", "x_002.png", "x_0001.png",
                                            "x_0002.png", "x_001.jpg", "x_002.jpg"};
    const auto items = group_images(files);
    CHECK_EQ(items.size(), std::size_t{3});
    for (const ScanItem& item : items) CHECK(item.is_sequence);
}

SVJ_TEST("library_scan: a sequence pattern's cache is named without the percent") {
    // "%04d" in a file name is legal and looks like a bug in every browser,
    // and ffmpeg would read the cache path as a pattern again.
    CHECK_EQ(cache_path_for("D:/shots/frame_%04d.png", ""),
             std::string("D:/shots/frame_seq.png.svcache"));
    const std::string hashed = cache_path_for("D:/shots/frame_%04d.png", "E:/cache");
    CHECK(hashed.find('%') == std::string::npos);
}
