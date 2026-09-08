#include <cstdio>
#include <string>

#include "core/bc1.h"
#include "core/cachemeta.h"
#include "core/videocache.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("cachemeta: the source path and the thumbnail survive a round trip") {
    CacheMeta meta;
    meta.source = "D:/rushes/tokyo.mp4";
    meta.thumb_w = 128;
    meta.thumb_h = 72;
    meta.thumb_bc1.assign(block_bytes_per_frame(128, 72, BlockFormat::BC1), std::uint8_t{0xAB});

    const std::vector<std::uint8_t> blob = encode_meta(meta);
    CHECK_EQ(blob.size(), meta_encoded_size(meta));
    const CacheMeta back = decode_meta(blob);
    CHECK_EQ(back.source, meta.source);
    CHECK(back.has_thumbnail());
    CHECK_EQ(back.thumb_w, std::uint32_t{128});
    CHECK_EQ(back.thumb_h, std::uint32_t{72});
    CHECK(back.thumb_bc1 == meta.thumb_bc1);
}

SVJ_TEST("cachemeta: A BARE PATH FROM AN OLDER CACHE IS STILL THE SOURCE") {
    // Every cache written before the thumbnail existed stores the path and
    // nothing else. Re-analysing a library to gain a picture would be the
    // wrong price, so that blob must keep meaning what it meant.
    const std::string path = "C:/clips/mon_rush.mp4";
    const std::vector<std::uint8_t> blob(path.begin(), path.end());
    const CacheMeta meta = decode_meta(blob);
    CHECK_EQ(meta.source, path);
    CHECK(!meta.has_thumbnail());
}

SVJ_TEST("cachemeta: a thumbnail of the wrong size is dropped and the path kept") {
    CacheMeta meta;
    meta.source = "x.mp4";
    meta.thumb_w = 128;
    meta.thumb_h = 72;
    meta.thumb_bc1.assign(10, std::uint8_t{1});  // not 32 x 18 blocks
    const CacheMeta back = decode_meta(encode_meta(meta));
    CHECK_EQ(back.source, std::string("x.mp4"));
    CHECK(!back.has_thumbnail());
}

SVJ_TEST("cachemeta: a truncated blob gives an empty meta rather than reading past the end") {
    CacheMeta meta;
    meta.source = "long/enough/path.mp4";
    std::vector<std::uint8_t> blob = encode_meta(meta);
    blob.resize(9);  // magic, length, one byte of path
    const CacheMeta back = decode_meta(blob);
    CHECK(back.source.empty());
    CHECK(!back.has_thumbnail());
}

SVJ_TEST("cachemeta: the thumbnail height keeps the aspect and tiles in whole blocks") {
    CHECK_EQ(thumbnail_height_for(1920, 1080), std::uint32_t{72});
    CHECK_EQ(thumbnail_height_for(3840, 1920), std::uint32_t{64});
    CHECK_EQ(thumbnail_height_for(640, 360), std::uint32_t{72});
    CHECK_EQ(thumbnail_height_for(1080, 1920), std::uint32_t{228});  // 227.6 -> 228
    CHECK_EQ(thumbnail_height_for(100, 1), std::uint32_t{4});
    CHECK_EQ(thumbnail_height_for(0, 0), std::uint32_t{4});
}

SVJ_TEST("cachemeta: the thumbnail is the picture, smaller") {
    // A frame that is red on the left and blue on the right: the thumbnail's
    // left and right blocks must decode to those colours.
    const std::uint32_t w = 640, h = 360;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * h * 4);
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            std::uint8_t* p = rgba.data() + (static_cast<std::size_t>(y) * w + x) * 4;
            p[0] = x < w / 2 ? 255 : 0;
            p[1] = 0;
            p[2] = x < w / 2 ? 0 : 255;
            p[3] = 255;
        }
    }
    CacheMeta meta;
    make_thumbnail(rgba.data(), w, h, meta);
    CHECK(meta.has_thumbnail());
    CHECK_EQ(meta.thumb_w, kThumbnailWidth);
    CHECK_EQ(meta.thumb_h, std::uint32_t{72});
    CHECK_EQ(meta.thumb_bc1.size(),
             static_cast<std::size_t>(block_bytes_per_frame(128, 72, BlockFormat::BC1)));

    std::vector<std::uint8_t> back;
    decode_bc1(meta.thumb_bc1.data(), meta.thumb_w, meta.thumb_h, back);
    const std::uint8_t* left = back.data() + (36 * 128 + 10) * 4;
    const std::uint8_t* right = back.data() + (36 * 128 + 118) * 4;
    CHECK(left[0] > 200 && left[2] < 40);
    CHECK(right[2] > 200 && right[0] < 40);
}

SVJ_TEST("cachemeta: A CACHE'S METADATA CAN BE REWRITTEN IN PLACE ONCE THE FRAMES ARE KNOWN") {
    // The blob's length is fixed at open, before a single frame is decoded,
    // so the thumbnail cannot be there yet. The writer takes a placeholder of
    // the final length and swaps the real blob in before closing.
    const std::string path = "test_cachemeta_rewrite.svcache";
    CacheHeader header;
    header.width = 8;
    header.height = 8;
    header.fps_num = 30;
    header.fps_den = 1;
    header.format = BlockFormat::BC1;

    CacheMeta placeholder;
    placeholder.source = "a.mp4";
    placeholder.thumb_w = 8;
    placeholder.thumb_h = 4;
    placeholder.thumb_bc1.assign(block_bytes_per_frame(8, 4, BlockFormat::BC1), std::uint8_t{0});

    std::string error;
    CacheWriter writer;
    CHECK(writer.open(path, header, encode_meta(placeholder), error));
    std::vector<std::uint8_t> frame(block_bytes_per_frame(8, 8, BlockFormat::BC1), std::uint8_t{0x5A});
    CHECK(writer.write_frame(frame.data(), frame.size(), error));

    CacheMeta real = placeholder;
    real.thumb_bc1.assign(real.thumb_bc1.size(), std::uint8_t{0xC3});
    CHECK(writer.rewrite_metadata(encode_meta(real), error));
    // A blob of another length would shift every frame: refused.
    CacheMeta longer = real;
    longer.source = "a-longer-name.mp4";
    CHECK(!writer.rewrite_metadata(encode_meta(longer), error));
    CHECK(writer.close(error));

    CacheReader reader;
    CHECK(reader.open(path, error));
    const CacheMeta back = decode_meta(reader.metadata());
    CHECK(back.has_thumbnail());
    CHECK_EQ(back.thumb_bc1[0], std::uint8_t{0xC3});
    std::vector<std::uint8_t> read;
    CHECK(reader.read_frame(0, read, error));
    CHECK_EQ(read[0], std::uint8_t{0x5A});
    reader.close();
    std::remove(path.c_str());
}
