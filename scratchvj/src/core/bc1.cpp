#include "core/bc1.h"

#include <algorithm>
#include <cstring>

namespace svj {
namespace {

// 8-bit channel to 5 or 6 bits and back, using the replication the hardware
// uses (r << 3 | r >> 2), so our decoder matches the GPU's bit for bit.
std::uint16_t pack565(int r, int g, int b) {
    return static_cast<std::uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void unpack565(std::uint16_t c, int& r, int& g, int& b) {
    const int r5 = (c >> 11) & 31;
    const int g6 = (c >> 5) & 63;
    const int b5 = c & 31;
    r = (r5 << 3) | (r5 >> 2);
    g = (g6 << 2) | (g6 >> 4);
    b = (b5 << 3) | (b5 >> 2);
}

struct Texel {
    int r, g, b;
};

// Gathers one 4x4 block, replicating edge pixels where the image ends. The
// clamp, not zero padding, is deliberate: a black phantom column would drag the
// endpoints of every right-edge block toward black and put a dark seam down the
// side of the picture.
void gather_block(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                  std::uint32_t bx, std::uint32_t by, Texel block[16]) {
    for (std::uint32_t y = 0; y < 4; ++y) {
        const std::uint32_t sy = std::min(by * 4 + y, height - 1);
        for (std::uint32_t x = 0; x < 4; ++x) {
            const std::uint32_t sx = std::min(bx * 4 + x, width - 1);
            const std::uint8_t* p = rgba + (static_cast<std::size_t>(sy) * width + sx) * 4;
            block[y * 4 + x] = Texel{p[0], p[1], p[2]};
        }
    }
}

// Endpoint choice: the bounding box of the block, inset by 1/16th. The inset is
// the classic trick -- extreme texels are usually noise, and pulling the
// endpoints in slightly spends the palette on the pixels that are actually
// common. Not the optimal encoder by a distance, but deterministic, fast, and
// good enough that the seam you notice on stage is the projector, not this.
void choose_endpoints(const Texel block[16], Texel& lo, Texel& hi) {
    lo = Texel{255, 255, 255};
    hi = Texel{0, 0, 0};
    for (int i = 0; i < 16; ++i) {
        lo.r = std::min(lo.r, block[i].r);
        lo.g = std::min(lo.g, block[i].g);
        lo.b = std::min(lo.b, block[i].b);
        hi.r = std::max(hi.r, block[i].r);
        hi.g = std::max(hi.g, block[i].g);
        hi.b = std::max(hi.b, block[i].b);
    }
    const int ir = (hi.r - lo.r) >> 4;
    const int ig = (hi.g - lo.g) >> 4;
    const int ib = (hi.b - lo.b) >> 4;
    lo.r += ir; lo.g += ig; lo.b += ib;
    hi.r -= ir; hi.g -= ig; hi.b -= ib;
}

int distance_sq(const Texel& a, const Texel& b) {
    const int dr = a.r - b.r;
    const int dg = a.g - b.g;
    const int db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

// Encodes one gathered block. The encoder proper; the frame function only
// walks the picture and gathers.
void encode_block(const Texel block[16], std::uint8_t* dst) {
    Texel palette[4];
    {
        {
            Texel lo, hi;
            choose_endpoints(block, lo, hi);
            std::uint16_t c0 = pack565(hi.r, hi.g, hi.b);
            std::uint16_t c1 = pack565(lo.r, lo.g, lo.b);

            // BC1 switches to its punch-through-alpha mode when c0 <= c1; we
            // never want that mode, so equal endpoints are split apart by one
            // bit and a swapped pair is swapped back.
            if (c0 < c1) std::swap(c0, c1);
            if (c0 == c1) {
                dst[0] = static_cast<std::uint8_t>(c0 & 0xFF);
                dst[1] = static_cast<std::uint8_t>(c0 >> 8);
                dst[2] = static_cast<std::uint8_t>(c1 & 0xFF);
                dst[3] = static_cast<std::uint8_t>(c1 >> 8);
                dst[4] = dst[5] = dst[6] = dst[7] = 0;  // every index -> c0
                return;
            }

            int r0, g0, b0, r1, g1, b1;
            unpack565(c0, r0, g0, b0);
            unpack565(c1, r1, g1, b1);
            palette[0] = Texel{r0, g0, b0};
            palette[1] = Texel{r1, g1, b1};
            palette[2] = Texel{(2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3};
            palette[3] = Texel{(r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3};

            std::uint32_t indices = 0;
            for (int i = 0; i < 16; ++i) {
                int best = 0;
                int best_d = distance_sq(block[i], palette[0]);
                for (int p = 1; p < 4; ++p) {
                    const int d = distance_sq(block[i], palette[p]);
                    if (d < best_d) {
                        best_d = d;
                        best = p;
                    }
                }
                indices |= static_cast<std::uint32_t>(best) << (i * 2);
            }

            dst[0] = static_cast<std::uint8_t>(c0 & 0xFF);
            dst[1] = static_cast<std::uint8_t>(c0 >> 8);
            dst[2] = static_cast<std::uint8_t>(c1 & 0xFF);
            dst[3] = static_cast<std::uint8_t>(c1 >> 8);
            dst[4] = static_cast<std::uint8_t>(indices & 0xFF);
            dst[5] = static_cast<std::uint8_t>((indices >> 8) & 0xFF);
            dst[6] = static_cast<std::uint8_t>((indices >> 16) & 0xFF);
            dst[7] = static_cast<std::uint8_t>((indices >> 24) & 0xFF);
        }
    }
}

// Reads one block's palette and index word. Edge clipping is the caller's.
void decode_block(const std::uint8_t* src, Texel palette[4], std::uint32_t& indices) {
    const std::uint16_t c0 = static_cast<std::uint16_t>(src[0] | (src[1] << 8));
    const std::uint16_t c1 = static_cast<std::uint16_t>(src[2] | (src[3] << 8));
    indices = static_cast<std::uint32_t>(src[4]) | (static_cast<std::uint32_t>(src[5]) << 8) |
              (static_cast<std::uint32_t>(src[6]) << 16) |
              (static_cast<std::uint32_t>(src[7]) << 24);

    int r0, g0, b0, r1, g1, b1;
    unpack565(c0, r0, g0, b0);
    unpack565(c1, r1, g1, b1);
    palette[0] = Texel{r0, g0, b0};
    palette[1] = Texel{r1, g1, b1};
    if (c0 > c1) {
        palette[2] = Texel{(2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3};
        palette[3] = Texel{(r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3};
    } else {
        // Punch-through mode. The encoder never writes it, but the decoder
        // honours it anyway: these bytes may one day come from a cache another
        // tool produced.
        palette[2] = Texel{(r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2};
        palette[3] = Texel{0, 0, 0};
    }
}

}  // namespace

std::uint64_t bc1_frame_bytes(std::uint32_t width, std::uint32_t height) {
    const std::uint64_t bw = (width + 3) / 4;
    const std::uint64_t bh = (height + 3) / 4;
    return bw * bh * 8;
}

void encode_bc1_block(const std::uint8_t* rgba16, std::uint8_t out[8]) {
    Texel block[16];
    for (int i = 0; i < 16; ++i) {
        block[i] = Texel{rgba16[i * 4], rgba16[i * 4 + 1], rgba16[i * 4 + 2]};
    }
    encode_block(block, out);
}

void decode_bc1_block(const std::uint8_t in[8], std::uint8_t* rgba16) {
    Texel palette[4];
    std::uint32_t indices = 0;
    decode_block(in, palette, indices);
    for (int i = 0; i < 16; ++i) {
        const Texel& t = palette[(indices >> (i * 2)) & 3];
        rgba16[i * 4] = static_cast<std::uint8_t>(t.r);
        rgba16[i * 4 + 1] = static_cast<std::uint8_t>(t.g);
        rgba16[i * 4 + 2] = static_cast<std::uint8_t>(t.b);
        rgba16[i * 4 + 3] = 255;
    }
}

void encode_bc1(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out) {
    const std::uint32_t bw = (width + 3) / 4;
    const std::uint32_t bh = (height + 3) / 4;
    out.resize(static_cast<std::size_t>(bw) * bh * 8);

    std::uint8_t* dst = out.data();
    Texel block[16];
    for (std::uint32_t by = 0; by < bh; ++by) {
        for (std::uint32_t bx = 0; bx < bw; ++bx, dst += 8) {
            gather_block(rgba, width, height, bx, by, block);
            encode_block(block, dst);
        }
    }
}

void decode_bc1(const std::uint8_t* bc1, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out) {
    const std::uint32_t bw = (width + 3) / 4;
    const std::uint32_t bh = (height + 3) / 4;
    out.resize(static_cast<std::size_t>(width) * height * 4);

    const std::uint8_t* src = bc1;
    Texel palette[4];

    for (std::uint32_t by = 0; by < bh; ++by) {
        for (std::uint32_t bx = 0; bx < bw; ++bx, src += 8) {
            std::uint32_t indices = 0;
            decode_block(src, palette, indices);

            for (std::uint32_t y = 0; y < 4; ++y) {
                const std::uint32_t py = by * 4 + y;
                if (py >= height) break;
                for (std::uint32_t x = 0; x < 4; ++x) {
                    const std::uint32_t px = bx * 4 + x;
                    if (px >= width) break;
                    const Texel& t = palette[(indices >> ((y * 4 + x) * 2)) & 3];
                    std::uint8_t* p =
                        out.data() + (static_cast<std::size_t>(py) * width + px) * 4;
                    p[0] = static_cast<std::uint8_t>(t.r);
                    p[1] = static_cast<std::uint8_t>(t.g);
                    p[2] = static_cast<std::uint8_t>(t.b);
                    p[3] = 255;
                }
            }
        }
    }
}

}  // namespace svj
