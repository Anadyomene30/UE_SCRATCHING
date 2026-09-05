#include <cstdint>
#include <cstdlib>
#include <vector>

#include "core/bc1.h"
#include "core/videocache.h"
#include "harness.h"

using namespace svj;

namespace {

std::vector<std::uint8_t> solid(std::uint32_t w, std::uint32_t h, std::uint8_t r,
                                std::uint8_t g, std::uint8_t b) {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < image.size(); i += 4) {
        image[i] = r;
        image[i + 1] = g;
        image[i + 2] = b;
        image[i + 3] = 255;
    }
    return image;
}

int max_channel_error(const std::vector<std::uint8_t>& a,
                      const std::vector<std::uint8_t>& b) {
    int worst = 0;
    for (std::size_t i = 0; i < a.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            const int d = std::abs(static_cast<int>(a[i + c]) - static_cast<int>(b[i + c]));
            worst = std::max(worst, d);
        }
    }
    return worst;
}

}  // namespace

SVJ_TEST("bc1: frame size agrees with the cache format's arithmetic") {
    // The cache addresses frames by multiplication, so the encoder and the format
    // must never disagree about what a frame occupies.
    CHECK_EQ(bc1_frame_bytes(1920, 1080),
             block_bytes_per_frame(1920, 1080, BlockFormat::BC1));
    CHECK_EQ(bc1_frame_bytes(1, 1), block_bytes_per_frame(1, 1, BlockFormat::BC1));
    CHECK_EQ(bc1_frame_bytes(1023, 511), block_bytes_per_frame(1023, 511, BlockFormat::BC1));
}

SVJ_TEST("bc1: a 565-representable solid colour round-trips exactly") {
    // Chosen ON the 565 lattice: a channel only survives quantisation when it
    // equals its own bit-replication, v == (q << 3) | (q >> 2). Multiples of 8
    // do NOT qualify -- 136 quantises to 17 and replicates back to 140 -- which
    // is exactly the mistake this test made in its first draft.
    const std::uint32_t w = 16, h = 16;
    const auto image = solid(w, h, 140, 130, 132);  // r5=17, g6=32, b5=16

    std::vector<std::uint8_t> packed, back;
    encode_bc1(image.data(), w, h, packed);
    decode_bc1(packed.data(), w, h, back);
    CHECK_EQ(max_channel_error(image, back), 0);
}

SVJ_TEST("bc1: a gradient survives within block-compression tolerance") {
    const std::uint32_t w = 64, h = 64;
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            std::uint8_t* p = image.data() + (static_cast<std::size_t>(y) * w + x) * 4;
            p[0] = static_cast<std::uint8_t>(x * 4);
            p[1] = static_cast<std::uint8_t>(y * 4);
            p[2] = static_cast<std::uint8_t>(255 - x * 2);
            p[3] = 255;
        }
    }

    std::vector<std::uint8_t> packed, back;
    encode_bc1(image.data(), w, h, packed);
    CHECK_EQ(packed.size(), bc1_frame_bytes(w, h));
    decode_bc1(packed.data(), w, h, back);

    // A 4x4 block of a smooth gradient spans few values, so a sane encoder stays
    // well inside this. The bound is loose on purpose: it asserts "compressed
    // picture", not one encoder's exact output.
    CHECK(max_channel_error(image, back) <= 24);
}

SVJ_TEST("bc1: odd dimensions pad with edge pixels, not with black") {
    // A black phantom column would drag every right-edge block toward black and
    // draw a dark seam down the side of the picture -- the bug this asserts
    // against. On a solid image, padding done right is invisible.
    const std::uint32_t w = 17, h = 9;  // neither a multiple of four
    const auto image = solid(w, h, 255, 162, 49);  // on the 565 lattice

    std::vector<std::uint8_t> packed, back;
    encode_bc1(image.data(), w, h, packed);
    CHECK_EQ(packed.size(), bc1_frame_bytes(w, h));
    decode_bc1(packed.data(), w, h, back);
    CHECK_EQ(back.size(), image.size());
    CHECK_EQ(max_channel_error(image, back), 0);
}

SVJ_TEST("bc1: encoding is deterministic") {
    // The analysis pass may be re-run at any time; an encoder with hidden state
    // would make every re-analysis a spurious diff.
    const std::uint32_t w = 32, h = 32;
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    std::uint32_t seed = 0x12345678;
    for (auto& byte : image) {
        seed = seed * 1664525u + 1013904223u;
        byte = static_cast<std::uint8_t>(seed >> 24);
    }

    std::vector<std::uint8_t> first, second;
    encode_bc1(image.data(), w, h, first);
    encode_bc1(image.data(), w, h, second);
    CHECK(first == second);
}

SVJ_TEST("bc1: decoded alpha is opaque everywhere") {
    // BC1 carries no alpha, and half-transparent garbage in the alpha channel
    // would composite as flicker once the mixer blends layers.
    const std::uint32_t w = 8, h = 8;
    const auto image = solid(w, h, 10, 200, 30);
    std::vector<std::uint8_t> packed, back;
    encode_bc1(image.data(), w, h, packed);
    decode_bc1(packed.data(), w, h, back);
    for (std::size_t i = 3; i < back.size(); i += 4) CHECK_EQ(back[i], 255);
}
