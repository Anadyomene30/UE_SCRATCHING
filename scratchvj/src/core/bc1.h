// scratchvj — BC1 block compression, both directions.
//
// The analysis pass writes .svcache frames in a GPU-native block format so that
// playback is an array lookup and upload, never a decode. BC1 is the starting
// format: 8 bytes per 4x4 block, a fixed 8:1 ratio over RGB, decoded in hardware
// by every GPU made this century. BC7 gives better quality and alpha but its
// encoder is a project in itself; BC1 is entirely writable — and therefore
// entirely testable — right here, with no dependency.
//
// The DECODER is not only for tests. Until the bgfx renderer exists, the front
// end shows frames through SDL's plain renderer, which cannot upload compressed
// textures — so the interface decodes the one frame it displays back to RGBA on
// the CPU. That is a stopgap with a visible seam, and it is exactly where bgfx
// will attach: same bytes, uploaded compressed instead of expanded.
#pragma once

#include <cstdint>
#include <vector>

namespace svj {

// Bytes a BC1 frame of this size occupies: 8 per 4x4 block, dimensions rounded
// up to whole blocks. Matches block_bytes_per_frame(..., BlockFormat::BC1).
std::uint64_t bc1_frame_bytes(std::uint32_t width, std::uint32_t height);

// Compresses an RGBA8 image (tightly packed, row-major, 4 bytes per pixel) into
// BC1. Edge blocks of a non-multiple-of-four image replicate their border
// pixels, so no out-of-image texel ever influences the encoding. Alpha is
// discarded: BC1 has no usable alpha, which is why the cache format marks
// alpha-carrying clips for BC3/BC7 instead.
void encode_bc1(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out);

// Expands a BC1 frame back to RGBA8 (alpha forced opaque). The exact inverse of
// the hardware's own decode, which is what makes round-trip tests meaningful.
void decode_bc1(const std::uint8_t* bc1, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out);

// One 4x4 block, already gathered: 16 RGBA texels in, 8 bytes out, and back.
// Exposed for BC3, whose colour half IS a BC1 block -- sharing the encoder is
// what keeps the two formats from drifting apart on the same picture.
void encode_bc1_block(const std::uint8_t* rgba16, std::uint8_t out[8]);
void decode_bc1_block(const std::uint8_t in[8], std::uint8_t* rgba16);

}  // namespace svj
