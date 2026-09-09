#include <cstdint>
#include <cstdlib>
#include <vector>

#include "core/bc1.h"
#include "core/bc3.h"
#include "core/videocache.h"
#include "harness.h"

using namespace svj;

namespace {

std::vector<std::uint8_t> solid(std::uint32_t w, std::uint32_t h, std::uint8_t r,
                                std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < image.size(); i += 4) {
        image[i] = r;
        image[i + 1] = g;
        image[i + 2] = b;
        image[i + 3] = a;
    }
    return image;
}

int max_alpha_error(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    int worst = 0;
    for (std::size_t i = 3; i < a.size(); i += 4) {
        worst = std::max(worst, std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i])));
    }
    return worst;
}

}  // namespace

SVJ_TEST("bc3: frame size agrees with the cache format's arithmetic") {
    // The cache addresses frames by multiplication; a disagreement here would
    // put every frame after the first at the wrong offset.
    CHECK_EQ(bc3_frame_bytes(1920, 1080), block_bytes_per_frame(1920, 1080, BlockFormat::BC3));
    CHECK_EQ(bc3_frame_bytes(1, 1), block_bytes_per_frame(1, 1, BlockFormat::BC3));
    CHECK_EQ(bc3_frame_bytes(1023, 511), block_bytes_per_frame(1023, 511, BlockFormat::BC3));
    CHECK_EQ(bc3_frame_bytes(16, 16), 2 * bc1_frame_bytes(16, 16));
}

SVJ_TEST("bc3: the colour half is exactly what BC1 would have written") {
    // BC3 must not be a second colour encoder. The same picture, opaque, has to
    // decode to the same colours either way -- otherwise a clip changes look
    // the moment it gains an alpha channel.
    const std::uint32_t w = 24, h = 20;
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    std::uint32_t seed = 7;
    for (std::size_t i = 0; i < image.size(); ++i) {
        seed = seed * 1664525u + 1013904223u;
        image[i] = static_cast<std::uint8_t>(seed >> 24);
    }
    for (std::size_t i = 3; i < image.size(); i += 4) image[i] = 255;

    std::vector<std::uint8_t> bc1, bc3, from_bc1, from_bc3;
    encode_bc1(image.data(), w, h, bc1);
    encode_bc3(image.data(), w, h, bc3);
    decode_bc1(bc1.data(), w, h, from_bc1);
    decode_bc3(bc3.data(), w, h, from_bc3);
    CHECK(from_bc1 == from_bc3);
}

SVJ_TEST("bc3: an alpha ramp round-trips within one step of the palette") {
    // Sixteen distinct alphas in one block, spread over 0..255: eight palette
    // entries cover them to within half a palette step. 255/7 is ~36, so the
    // worst case is 18 -- and a coarser bound would hide a broken interpolant.
    const std::uint32_t w = 4, h = 4;
    std::vector<std::uint8_t> image(64);
    for (int i = 0; i < 16; ++i) {
        image[i * 4] = 120;
        image[i * 4 + 1] = 90;
        image[i * 4 + 2] = 200;
        image[i * 4 + 3] = static_cast<std::uint8_t>(i * 17);
    }
    std::vector<std::uint8_t> packed, back;
    encode_bc3(image.data(), w, h, packed);
    decode_bc3(packed.data(), w, h, back);
    CHECK(max_alpha_error(image, back) <= 19);
    // The endpoints themselves are exact.
    CHECK_EQ(static_cast<int>(back[3]), 0);
    CHECK_EQ(static_cast<int>(back[15 * 4 + 3]), 255);
}

SVJ_TEST("bc3: fully transparent and fully opaque blocks are exact") {
    // The two cases a logo is made of, and the ones where any error is visible
    // as a halo or a hole.
    const std::uint32_t w = 8, h = 8;
    for (const std::uint8_t a : {std::uint8_t{0}, std::uint8_t{255}, std::uint8_t{128}}) {
        const auto image = solid(w, h, 140, 130, 132, a);
        std::vector<std::uint8_t> packed, back;
        encode_bc3(image.data(), w, h, packed);
        decode_bc3(packed.data(), w, h, back);
        CHECK_EQ(max_alpha_error(image, back), 0);
    }
}

SVJ_TEST("bc3: edge blocks of an odd-sized picture stay inside it") {
    // A 5x3 picture is one block wide and one high with most texels off the
    // edge; the decoder must write exactly 5x3 pixels and the encoder must not
    // read outside the input.
    const std::uint32_t w = 5, h = 3;
    const auto image = solid(w, h, 10, 20, 30, 77);
    std::vector<std::uint8_t> packed, back;
    encode_bc3(image.data(), w, h, packed);
    CHECK_EQ(packed.size(), std::size_t{2 * 16});
    decode_bc3(packed.data(), w, h, back);
    CHECK_EQ(back.size(), std::size_t{5 * 3 * 4});
    CHECK_EQ(max_alpha_error(image, back), 0);
}

SVJ_TEST("bc3: the decoder honours the six-entry alpha mode another tool may write") {
    // a0 <= a1 selects the mode with fixed 0 and 255 slots. Our encoder never
    // writes it, but a cache made elsewhere might; the decoder must not treat
    // those bytes as the other mode.
    std::uint8_t block[16] = {};
    block[0] = 100;  // a0
    block[1] = 200;  // a1, greater: six-entry mode
    // Index 6 for every texel -> 0, then index 7 -> 255.
    std::uint64_t indices = 0;
    for (int i = 0; i < 16; ++i) indices |= std::uint64_t{6} << (i * 3);
    for (int b = 0; b < 6; ++b) block[2 + b] = static_cast<std::uint8_t>((indices >> (b * 8)) & 0xFF);
    std::vector<std::uint8_t> back;
    decode_bc3(block, 4, 4, back);
    CHECK_EQ(static_cast<int>(back[3]), 0);

    indices = 0;
    for (int i = 0; i < 16; ++i) indices |= std::uint64_t{7} << (i * 3);
    for (int b = 0; b < 6; ++b) block[2 + b] = static_cast<std::uint8_t>((indices >> (b * 8)) & 0xFF);
    decode_bc3(block, 4, 4, back);
    CHECK_EQ(static_cast<int>(back[3]), 255);
}
