#include <cstdio>
#include <numeric>

#include "core/videocache.h"
#include "harness.h"

using namespace svj;

namespace {

CacheHeader header_1080p(BlockFormat format = BlockFormat::BC7) {
    CacheHeader h;
    h.width = 1920;
    h.height = 1080;
    h.fps_num = 60;
    h.fps_den = 1;
    h.format = format;
    return h;
}

std::vector<std::uint8_t> frame_of(std::size_t bytes, std::uint8_t fill) {
    return std::vector<std::uint8_t>(bytes, fill);
}

struct TempFile {
    explicit TempFile(std::string name) : path(std::move(name)) {}
    ~TempFile() { std::remove(path.c_str()); }
    std::string path;
};

}  // namespace

SVJ_TEST("cache: block sizes follow the format and round up to whole blocks") {
    // 1920x1080 is 480 x 270 blocks.
    CHECK_EQ(block_bytes_per_frame(1920, 1080, BlockFormat::BC1), std::uint64_t{480 * 270 * 8});
    CHECK_EQ(block_bytes_per_frame(1920, 1080, BlockFormat::BC7), std::uint64_t{480 * 270 * 16});
    CHECK_EQ(block_bytes_per_frame(1920, 1080, BlockFormat::BC3), std::uint64_t{480 * 270 * 16});

    // A size that is not a multiple of four still costs whole blocks.
    CHECK_EQ(block_bytes_per_frame(5, 5, BlockFormat::BC1), std::uint64_t{2 * 2 * 8});
    CHECK_EQ(block_bytes_per_frame(1, 1, BlockFormat::BC7), std::uint64_t{16});
}

SVJ_TEST("cache: the plan's VRAM budget table holds up") {
    // 1 GiB per deck, as the plan quotes it.
    const std::uint64_t budget = 1ull << 30;
    const std::uint64_t hd = block_bytes_per_frame(1280, 720, BlockFormat::BC1);
    const std::uint64_t equirect4k = block_bytes_per_frame(3840, 1920, BlockFormat::BC1);

    CHECK_NEAR(static_cast<double>(hd) / 1e6, 0.46, 0.02);          // ~0.46 MB
    CHECK_NEAR(static_cast<double>(equirect4k) / 1e6, 3.7, 0.2);    // ~3.7 MB

    // Seconds at 60 fps, which is what the table promises.
    CHECK_NEAR(static_cast<double>(budget / hd) / 60.0, 38.0, 4.0);
    CHECK_NEAR(static_cast<double>(budget / equirect4k) / 60.0, 4.8, 1.0);
}

SVJ_TEST("cache: timing converts both ways") {
    CacheHeader h = header_1080p();
    h.frame_count = 600;  // 10 s at 60 fps
    CHECK_NEAR(h.frame_duration_s(), 1.0 / 60.0, 1e-9);
    CHECK_NEAR(h.duration_s(), 10.0, 1e-9);
    CHECK_EQ(h.frame_at(0.0), std::uint32_t{0});
    CHECK_EQ(h.frame_at(1.0), std::uint32_t{60});
    CHECK_NEAR(h.time_of(60), 1.0, 1e-9);
}

SVJ_TEST("cache: a time outside the clip clamps rather than running off the end") {
    CacheHeader h = header_1080p();
    h.frame_count = 100;
    CHECK_EQ(h.frame_at(-5.0), std::uint32_t{0});
    CHECK_EQ(h.frame_at(1e6), std::uint32_t{99});
}

SVJ_TEST("cache: non-integer frame rates are handled exactly") {
    CacheHeader h;
    h.width = 640; h.height = 360;
    h.fps_num = 30000; h.fps_den = 1001;  // 29.97
    h.frame_count = 30000;
    CHECK_NEAR(h.frame_duration_s(), 1001.0 / 30000.0, 1e-12);

    // A time worth exactly N frames must land on N, not on N - 1. Dividing by a
    // rounded frame duration gets this wrong at 29.97 often enough to matter.
    for (const std::uint32_t frame : {std::uint32_t{1}, std::uint32_t{30},
                                      std::uint32_t{1000}, std::uint32_t{29999}}) {
        CHECK_EQ(h.frame_at(h.time_of(frame)), frame);
    }
    CHECK_EQ(h.frame_at(1.001), std::uint32_t{30});
}

SVJ_TEST("cache: a clip survives a write and read round trip") {
    TempFile temp("svj_cache_roundtrip.svcache");
    CacheHeader h;
    h.width = 64; h.height = 32; h.fps_num = 30; h.fps_den = 1;
    h.format = BlockFormat::BC7;
    h.flags = kCacheAlpha | kCacheEquirect;

    const std::uint64_t bytes = block_bytes_per_frame(64, 32, BlockFormat::BC7);
    std::vector<std::uint8_t> metadata{'b', 'e', 'a', 't', 's'};

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(temp.path, h, metadata, error));
    for (int i = 0; i < 5; ++i) {
        CHECK(writer.write_frame(frame_of(bytes, static_cast<std::uint8_t>(i)).data(), bytes, error));
    }
    CHECK_EQ(writer.frames_written(), std::uint32_t{5});
    CHECK(writer.close(error));

    CacheReader reader;
    CHECK(reader.open(temp.path, error));
    CHECK_EQ(reader.header().width, std::uint32_t{64});
    CHECK_EQ(reader.header().frame_count, std::uint32_t{5});
    CHECK(reader.header().has_alpha());
    CHECK(reader.header().is_equirect());
    CHECK_EQ(reader.frame_bytes(), bytes);
    CHECK_EQ(reader.metadata().size(), std::size_t{5});
    CHECK_EQ(std::string(reader.metadata().begin(), reader.metadata().end()), std::string("beats"));

    // Random access, in an order no stream could serve cheaply.
    for (const std::uint32_t index : {std::uint32_t{4}, std::uint32_t{0}, std::uint32_t{2}}) {
        std::vector<std::uint8_t> frame;
        CHECK(reader.read_frame(index, frame, error));
        CHECK_EQ(frame.size(), std::size_t{bytes});
        CHECK_EQ(int(frame[0]), int(index));
        CHECK_EQ(int(frame[bytes - 1]), int(index));
    }
}

SVJ_TEST("cache: a contiguous range reads in one go") {
    TempFile temp("svj_cache_range.svcache");
    CacheHeader h;
    h.width = 16; h.height = 16; h.fps_num = 25; h.fps_den = 1;
    h.format = BlockFormat::BC1;
    const std::uint64_t bytes = block_bytes_per_frame(16, 16, BlockFormat::BC1);

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(temp.path, h, {}, error));
    for (int i = 0; i < 10; ++i) {
        CHECK(writer.write_frame(frame_of(bytes, static_cast<std::uint8_t>(i)).data(), bytes, error));
    }
    CHECK(writer.close(error));

    CacheReader reader;
    CHECK(reader.open(temp.path, error));
    std::vector<std::uint8_t> run;
    CHECK(reader.read_range(3, 4, run, error));
    CHECK_EQ(run.size(), std::size_t{bytes * 4});
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK_EQ(int(run[i * bytes]), int(3 + i));
    }
}

SVJ_TEST("cache: a frame of the wrong size is refused instead of shifting the rest") {
    // A short frame would silently displace every later frame, and the damage
    // would only show up as a wrong picture much later.
    TempFile temp("svj_cache_shortframe.svcache");
    CacheHeader h = header_1080p();
    const std::uint64_t bytes = block_bytes_per_frame(h.width, h.height, h.format);

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(temp.path, h, {}, error));
    auto short_frame = frame_of(bytes - 1, 0);
    CHECK(!writer.write_frame(short_frame.data(), short_frame.size(), error));
    CHECK(error.find("expected") != std::string::npos);
    CHECK_EQ(writer.frames_written(), std::uint32_t{0});
    CHECK(writer.close(error));
}

SVJ_TEST("cache: an interrupted analysis pass is detected, not played as garbage") {
    TempFile temp("svj_cache_truncated.svcache");
    CacheHeader h;
    h.width = 32; h.height = 32; h.fps_num = 30; h.fps_den = 1;
    h.format = BlockFormat::BC1;
    const std::uint64_t bytes = block_bytes_per_frame(32, 32, BlockFormat::BC1);

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(temp.path, h, {}, error));
    for (int i = 0; i < 8; ++i) {
        CHECK(writer.write_frame(frame_of(bytes, 7).data(), bytes, error));
    }
    CHECK(writer.close(error));

    // Chop the tail off, as an interrupted analysis would.
    {
        std::FILE* f = std::fopen(temp.path.c_str(), "rb");
        std::vector<std::uint8_t> all;
        std::uint8_t buffer[4096];
        std::size_t got = 0;
        while ((got = std::fread(buffer, 1, sizeof(buffer), f)) > 0) {
            all.insert(all.end(), buffer, buffer + got);
        }
        std::fclose(f);
        all.resize(all.size() - bytes);
        std::FILE* out = std::fopen(temp.path.c_str(), "wb");
        std::fwrite(all.data(), 1, all.size(), out);
        std::fclose(out);
    }

    CacheReader reader;
    CHECK(!reader.open(temp.path, error));
    CHECK(error.find("interrupted") != std::string::npos);
}

SVJ_TEST("cache: a foreign or corrupt file is rejected with a reason") {
    TempFile temp("svj_cache_foreign.svcache");
    {
        std::FILE* f = std::fopen(temp.path.c_str(), "wb");
        const char junk[128] = "this is not a cache file at all, not even close";
        std::fwrite(junk, 1, sizeof(junk), f);
        std::fclose(f);
    }
    CacheReader reader;
    std::string error;
    CHECK(!reader.open(temp.path, error));
    CHECK(error.find("not a scratchvj cache") != std::string::npos);
}

SVJ_TEST("cache: a file too short to hold a header is rejected") {
    TempFile temp("svj_cache_stub.svcache");
    {
        std::FILE* f = std::fopen(temp.path.c_str(), "wb");
        std::fwrite("SVC1", 1, 4, f);
        std::fclose(f);
    }
    CacheReader reader;
    std::string error;
    CHECK(!reader.open(temp.path, error));
    CHECK(error.find("too short") != std::string::npos);
}

SVJ_TEST("cache: reading outside the clip fails rather than clamping") {
    TempFile temp("svj_cache_range_guard.svcache");
    CacheHeader h;
    h.width = 8; h.height = 8; h.fps_num = 30; h.fps_den = 1; h.format = BlockFormat::BC1;
    const std::uint64_t bytes = block_bytes_per_frame(8, 8, BlockFormat::BC1);

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(temp.path, h, {}, error));
    for (int i = 0; i < 3; ++i) CHECK(writer.write_frame(frame_of(bytes, 1).data(), bytes, error));
    CHECK(writer.close(error));

    CacheReader reader;
    CHECK(reader.open(temp.path, error));
    std::vector<std::uint8_t> frame;
    CHECK(!reader.read_frame(3, frame, error));
    CHECK(!reader.read_range(2, 5, frame, error));
    CHECK(reader.read_range(0, 3, frame, error));
}

SVJ_TEST("cache: an impossible header is refused before anything is played") {
    std::string error;
    CacheWriter writer;
    CacheHeader zero;
    CHECK(!writer.open("svj_cache_never.svcache", zero, {}, error));

    CacheHeader no_fps = header_1080p();
    no_fps.fps_num = 0;
    CHECK(!writer.open("svj_cache_never.svcache", no_fps, {}, error));
    CHECK(error.find("frame rate") != std::string::npos);
}
