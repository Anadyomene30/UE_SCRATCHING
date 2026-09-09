#include "core/bc3.h"

#include <algorithm>
#include <cstdlib>

#include "core/bc1.h"

namespace svj {
namespace {

// Gathers one 4x4 block of RGBA, replicating the border where the picture
// ends -- the same clamp BC1 uses, for the same reason: a phantom transparent
// column would pull every right-edge alpha endpoint toward zero and fringe the
// picture.
void gather_block(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                  std::uint32_t bx, std::uint32_t by, std::uint8_t block[64]) {
    for (std::uint32_t y = 0; y < 4; ++y) {
        const std::uint32_t sy = std::min(by * 4 + y, height - 1);
        for (std::uint32_t x = 0; x < 4; ++x) {
            const std::uint32_t sx = std::min(bx * 4 + x, width - 1);
            const std::uint8_t* p = rgba + (static_cast<std::size_t>(sy) * width + sx) * 4;
            std::uint8_t* q = block + (y * 4 + x) * 4;
            q[0] = p[0];
            q[1] = p[1];
            q[2] = p[2];
            q[3] = p[3];
        }
    }
}

// The eight-entry alpha palette of the a0 > a1 mode, exactly as the hardware
// interpolates it: integer division, no rounding term. Direct3D's BC4 spec.
void alpha_palette(int a0, int a1, int out[8]) {
    out[0] = a0;
    out[1] = a1;
    if (a0 > a1) {
        for (int i = 1; i <= 6; ++i) out[i + 1] = ((7 - i) * a0 + i * a1) / 7;
    } else {
        for (int i = 1; i <= 4; ++i) out[i + 1] = ((5 - i) * a0 + i * a1) / 5;
        out[6] = 0;
        out[7] = 255;
    }
}

void encode_alpha_block(const std::uint8_t block[64], std::uint8_t* dst) {
    int lo = 255, hi = 0;
    for (int i = 0; i < 16; ++i) {
        lo = std::min(lo, static_cast<int>(block[i * 4 + 3]));
        hi = std::max(hi, static_cast<int>(block[i * 4 + 3]));
    }

    // Always the eight-entry mode (a0 > a1), which spends all eight slots on
    // the block's own range. The six-entry mode's free 0 and 255 only pay off
    // on blocks that mix full transparency with mid alphas, and a fixed choice
    // keeps the encoder deterministic and testable. A flat block (lo == hi)
    // would give a0 == a1 and so the six-entry mode: every index then points at
    // a0, which is still exactly the block's alpha, so nothing is lost.
    const int a0 = hi;
    const int a1 = lo;
    int palette[8];
    alpha_palette(a0, a1, palette);

    std::uint64_t indices = 0;
    for (int i = 0; i < 16; ++i) {
        const int a = block[i * 4 + 3];
        int best = 0;
        int best_d = std::abs(a - palette[0]);
        const int count = a0 > a1 ? 8 : 6;  // never the constant 0/255 slots
        for (int p = 1; p < count; ++p) {
            const int d = std::abs(a - palette[p]);
            if (d < best_d) {
                best_d = d;
                best = p;
            }
        }
        indices |= static_cast<std::uint64_t>(best) << (i * 3);
    }

    dst[0] = static_cast<std::uint8_t>(a0);
    dst[1] = static_cast<std::uint8_t>(a1);
    for (int b = 0; b < 6; ++b) {
        dst[2 + b] = static_cast<std::uint8_t>((indices >> (b * 8)) & 0xFF);
    }
}

void decode_alpha_block(const std::uint8_t* src, std::uint8_t alpha[16]) {
    int palette[8];
    alpha_palette(src[0], src[1], palette);
    std::uint64_t indices = 0;
    for (int b = 0; b < 6; ++b) {
        indices |= static_cast<std::uint64_t>(src[2 + b]) << (b * 8);
    }
    for (int i = 0; i < 16; ++i) {
        alpha[i] = static_cast<std::uint8_t>(palette[(indices >> (i * 3)) & 7]);
    }
}

}  // namespace

std::uint64_t bc3_frame_bytes(std::uint32_t width, std::uint32_t height) {
    const std::uint64_t bw = (width + 3) / 4;
    const std::uint64_t bh = (height + 3) / 4;
    return bw * bh * 16;
}

void encode_bc3(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out) {
    const std::uint32_t bw = (width + 3) / 4;
    const std::uint32_t bh = (height + 3) / 4;
    out.resize(static_cast<std::size_t>(bw) * bh * 16);

    std::uint8_t* dst = out.data();
    std::uint8_t block[64];
    for (std::uint32_t by = 0; by < bh; ++by) {
        for (std::uint32_t bx = 0; bx < bw; ++bx, dst += 16) {
            gather_block(rgba, width, height, bx, by, block);
            encode_alpha_block(block, dst);
            encode_bc1_block(block, dst + 8);
        }
    }
}

void decode_bc3(const std::uint8_t* bc3, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out) {
    const std::uint32_t bw = (width + 3) / 4;
    const std::uint32_t bh = (height + 3) / 4;
    out.resize(static_cast<std::size_t>(width) * height * 4);

    const std::uint8_t* src = bc3;
    std::uint8_t alpha[16];
    std::uint8_t colour[64];
    for (std::uint32_t by = 0; by < bh; ++by) {
        for (std::uint32_t bx = 0; bx < bw; ++bx, src += 16) {
            decode_alpha_block(src, alpha);
            decode_bc1_block(src + 8, colour);
            for (std::uint32_t y = 0; y < 4; ++y) {
                const std::uint32_t py = by * 4 + y;
                if (py >= height) break;
                for (std::uint32_t x = 0; x < 4; ++x) {
                    const std::uint32_t px = bx * 4 + x;
                    if (px >= width) break;
                    const std::uint8_t* c = colour + (y * 4 + x) * 4;
                    std::uint8_t* p = out.data() + (static_cast<std::size_t>(py) * width + px) * 4;
                    p[0] = c[0];
                    p[1] = c[1];
                    p[2] = c[2];
                    p[3] = alpha[y * 4 + x];
                }
            }
        }
    }
}

}  // namespace svj
