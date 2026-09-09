// scratchvj — what the .svcache metadata block holds.
//
// The format reserves an opaque blob between the header and the frames
// (core/videocache), and until now that blob was the source path, bare. The
// library wants a picture of each clip, and the picture belongs in the cache:
// it is derived from the same decode, it survives the source moving, and it
// costs nothing to read back next to the header.
//
// So the blob gets a shape -- a small magic, the source path, and a BC1
// thumbnail -- while a blob WITHOUT the magic is still read as a bare path,
// because every cache written before this exists and re-analysing a library
// to gain a picture would be the wrong price.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace svj {

// The thumbnail's width. 128 keeps a 16:9 picture at 128x72 and a 2:1
// equirect at 128x64: both multiples of four, so the BC1 blocks tile with no
// padding and the bytes go to the GPU as they are.
inline constexpr std::uint32_t kThumbnailWidth = 128;

struct CacheMeta {
    std::string source;
    std::uint32_t thumb_w = 0;
    std::uint32_t thumb_h = 0;
    std::vector<std::uint8_t> thumb_bc1;  // block_bytes_per_frame(w, h, BC1) bytes, or empty

    bool has_thumbnail() const { return thumb_w > 0 && thumb_h > 0 && !thumb_bc1.empty(); }
};

// The height a source of `width` x `height` gets at kThumbnailWidth, rounded
// to a multiple of four (at least four). The analysis pass sizes its blob
// with this BEFORE decoding, since the blob's length is fixed at open.
std::uint32_t thumbnail_height_for(std::uint32_t width, std::uint32_t height);

// Bytes the encoded blob takes, so a placeholder of the right length can be
// written first and the real one rewritten in place later.
std::size_t meta_encoded_size(const CacheMeta& meta);

std::vector<std::uint8_t> encode_meta(const CacheMeta& meta);

// Reads either form: the tagged blob, or a bare source path from an older
// cache. Never fails -- a blob it cannot make sense of is an empty meta.
CacheMeta decode_meta(const std::vector<std::uint8_t>& blob);

// Box-filters an RGBA picture down to kThumbnailWidth x thumbnail_height_for()
// and BC1-encodes it into `meta`. `rgba` is width*height*4 bytes.
void make_thumbnail(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                    CacheMeta& meta);

}  // namespace svj
