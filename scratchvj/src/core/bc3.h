// scratchvj — BC3 block compression, both directions: BC1 colour plus alpha.
//
// A BC3 block is 16 bytes: an 8-byte ALPHA block (two endpoints and sixteen
// 3-bit indices, the BC4 layout) followed by the very same 8-byte colour block
// BC1 writes. The colour half is therefore not re-implemented -- it calls the
// BC1 block encoder, so a clip with alpha and the same clip without it compress
// their colours identically, and the only thing alpha costs is the alpha.
//
// This is what makes a logo, a mask or a ProRes 4444 overlay possible: BC1 has
// no usable alpha, and until now the analysis pass silently threw the channel
// away. The format doc always reserved BC3 for exactly this; here is the
// encoder it was waiting for.
#pragma once

#include <cstdint>
#include <vector>

namespace svj {

// Bytes a BC3 frame of this size occupies: 16 per 4x4 block. Matches
// block_bytes_per_frame(..., BlockFormat::BC3).
std::uint64_t bc3_frame_bytes(std::uint32_t width, std::uint32_t height);

// Compresses an RGBA8 image into BC3. Edge blocks replicate their border
// pixels, as BC1 does, so nothing outside the picture leaks into it.
void encode_bc3(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out);

// Expands a BC3 frame back to RGBA8. The exact inverse of the hardware decode
// (integer palette arithmetic as Direct3D specifies it), so round-trip tests
// measure the encoder, not a decoder of our own invention.
void decode_bc3(const std::uint8_t* bc3, std::uint32_t width, std::uint32_t height,
                std::vector<std::uint8_t>& out);

}  // namespace svj
