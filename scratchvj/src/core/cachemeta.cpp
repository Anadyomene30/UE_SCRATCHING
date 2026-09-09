#include "core/cachemeta.h"

#include <algorithm>
#include <cstring>

#include "core/bc1.h"
#include "core/bytes.h"
#include "core/videocache.h"

namespace svj {
namespace {

// "SVM1", little-endian, like the cache magic. Not a printable prefix a path
// could start with: a Windows path begins with a drive letter or a slash.
constexpr std::uint32_t kMetaMagic = 0x314D5653;

}  // namespace

std::uint32_t thumbnail_height_for(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) return 4;
    const std::uint64_t scaled =
        (static_cast<std::uint64_t>(height) * kThumbnailWidth + width / 2) / width;
    const std::uint32_t rounded = static_cast<std::uint32_t>((scaled + 2) / 4 * 4);
    return std::max<std::uint32_t>(4, rounded);
}

std::size_t meta_encoded_size(const CacheMeta& meta) {
    return 4 + 4 + meta.source.size() + 4 + 4 + 4 + meta.thumb_bc1.size();
}

std::vector<std::uint8_t> encode_meta(const CacheMeta& meta) {
    std::vector<std::uint8_t> out;
    out.reserve(meta_encoded_size(meta));
    put_u32(out, kMetaMagic);
    put_u32(out, static_cast<std::uint32_t>(meta.source.size()));
    out.insert(out.end(), meta.source.begin(), meta.source.end());
    put_u32(out, meta.thumb_w);
    put_u32(out, meta.thumb_h);
    put_u32(out, static_cast<std::uint32_t>(meta.thumb_bc1.size()));
    out.insert(out.end(), meta.thumb_bc1.begin(), meta.thumb_bc1.end());
    return out;
}

CacheMeta decode_meta(const std::vector<std::uint8_t>& blob) {
    CacheMeta meta;
    ByteReader head(blob.data(), blob.size());
    if (blob.size() < 4 || head.u32() != kMetaMagic) {
        // An older cache: the blob is the source path and nothing else.
        meta.source.assign(blob.begin(), blob.end());
        return meta;
    }
    ByteReader reader(blob.data(), blob.size());
    reader.u32();  // the magic, already checked
    const std::uint32_t source_len = reader.u32();
    meta.source = reader.str(source_len);
    if (!reader.ok()) return CacheMeta{};
    meta.thumb_w = reader.u32();
    meta.thumb_h = reader.u32();
    const std::uint32_t thumb_len = reader.u32();
    // A thumbnail that is not exactly its BC1 size is a thumbnail nobody can
    // upload: dropped, and the source kept.
    const bool sized = reader.ok() && meta.thumb_w > 0 && meta.thumb_h > 0 &&
                       thumb_len == block_bytes_per_frame(meta.thumb_w, meta.thumb_h, BlockFormat::BC1);
    const std::string bytes = sized ? reader.str(thumb_len) : std::string();
    if (sized && reader.ok()) {
        meta.thumb_bc1.assign(bytes.begin(), bytes.end());
    } else {
        meta.thumb_w = meta.thumb_h = 0;
    }
    return meta;
}

void make_thumbnail(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                    CacheMeta& meta) {
    const std::uint32_t tw = kThumbnailWidth;
    const std::uint32_t th = thumbnail_height_for(width, height);
    std::vector<std::uint8_t> small(static_cast<std::size_t>(tw) * th * 4);
    if (width == 0 || height == 0) {
        std::fill(small.begin(), small.end(), std::uint8_t{0});
    } else {
        // Box filter over the source pixels each destination pixel covers.
        // Not a resampler anyone would frame a print with, but a 128 px
        // picture in a list is a reminder of a clip, not the clip.
        for (std::uint32_t y = 0; y < th; ++y) {
            const std::uint32_t y0 = static_cast<std::uint32_t>(static_cast<std::uint64_t>(y) * height / th);
            const std::uint32_t y1 = std::max(y0 + 1, static_cast<std::uint32_t>(static_cast<std::uint64_t>(y + 1) * height / th));
            for (std::uint32_t x = 0; x < tw; ++x) {
                const std::uint32_t x0 = static_cast<std::uint32_t>(static_cast<std::uint64_t>(x) * width / tw);
                const std::uint32_t x1 = std::max(x0 + 1, static_cast<std::uint32_t>(static_cast<std::uint64_t>(x + 1) * width / tw));
                std::uint64_t sum[4] = {0, 0, 0, 0};
                std::uint64_t n = 0;
                for (std::uint32_t sy = y0; sy < std::min(y1, height); ++sy) {
                    for (std::uint32_t sx = x0; sx < std::min(x1, width); ++sx) {
                        const std::uint8_t* p = rgba + (static_cast<std::size_t>(sy) * width + sx) * 4;
                        for (int c = 0; c < 4; ++c) sum[c] += p[c];
                        ++n;
                    }
                }
                std::uint8_t* q = small.data() + (static_cast<std::size_t>(y) * tw + x) * 4;
                for (int c = 0; c < 4; ++c) {
                    q[c] = n == 0 ? 0 : static_cast<std::uint8_t>(sum[c] / n);
                }
            }
        }
    }
    meta.thumb_w = tw;
    meta.thumb_h = th;
    encode_bc1(small.data(), tw, th, meta.thumb_bc1);
}

}  // namespace svj
