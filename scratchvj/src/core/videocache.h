// scratchvj — the .svcache clip format.
//
// Scratching a video cannot go through a normal player: seeking a compressed
// stream will never keep up with a hand on a platter. So the clip is decoded ONCE,
// offline, into GPU-native block-compressed frames, and playback becomes an array
// lookup. That analysis pass is the same idea as Serato analysing a track before
// you play it.
//
// Frames are FIXED SIZE, because block compression makes them so. That is not a
// limitation, it is the point: the offset of frame n is arithmetic, so there is no
// index to consult, no lookup to miss, and no difference in cost between playing
// forwards, backwards, or jumping about. Reverse playback is free.
//
// Layout:
//
//   [ header, 64 bytes ]
//   [ metadata: u32 length + that many bytes ]   thumbnail, beatgrid, source path
//   [ frame data: frame_count x frame_bytes ]
//
// Little-endian throughout, written byte by byte (see core/bytes.h).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace svj {

inline constexpr std::uint32_t kCacheMagic = 0x31435653;  // "SVC1" little-endian
inline constexpr std::uint16_t kCacheVersion = 1;
inline constexpr std::size_t kCacheHeaderBytes = 64;

// Block-compressed formats. BC1 is the smallest and has no usable alpha; BC7 is
// the quality choice; BC3 carries alpha at half BC7's encode cost. The analysis
// pass runs offline, so quality is affordable.
enum class BlockFormat : std::uint8_t {
    BC1 = 1,  // 8 bytes per 4x4 block, no alpha
    BC3 = 2,  // 16 bytes per 4x4 block, alpha
    BC7 = 3,  // 16 bytes per 4x4 block, alpha, best quality
};

enum CacheFlag : std::uint16_t {
    kCacheAlpha = 1u << 0,      // the clip carries a meaningful alpha channel
    kCacheEquirect = 1u << 1,   // 360 footage, to be reprojected rather than shown flat
};

struct CacheHeader {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t frame_count = 0;
    std::uint32_t fps_num = 0;
    std::uint32_t fps_den = 1;
    BlockFormat format = BlockFormat::BC7;
    std::uint16_t flags = 0;

    bool has_alpha() const { return (flags & kCacheAlpha) != 0; }
    bool is_equirect() const { return (flags & kCacheEquirect) != 0; }

    double duration_s() const;
    double frame_duration_s() const;

    // Frame index for a time, clamped into the clip. Playback position is a time,
    // but the cache is addressed by frame, and this is the only place that
    // conversion happens.
    std::uint32_t frame_at(double seconds) const;
    double time_of(std::uint32_t frame) const;
};

// Bytes one frame occupies. Block compression pads to whole 4x4 blocks, so a
// 1920x1080 BC7 frame covers 1920x1080 rounded up to 1920x1080 (270 block rows).
std::uint64_t block_bytes_per_frame(std::uint32_t width, std::uint32_t height,
                                    BlockFormat format);

// Writes a .svcache. Frames are appended in order; every frame must be exactly
// block_bytes_per_frame() long, which the writer enforces rather than trusting.
class CacheWriter {
public:
    // `metadata` is an opaque blob (thumbnail, beatgrid, source path) the writer
    // stores verbatim and the reader hands back untouched.
    bool open(const std::string& path, const CacheHeader& header,
              const std::vector<std::uint8_t>& metadata, std::string& error);

    bool write_frame(const std::uint8_t* data, std::size_t size, std::string& error);

    // Rewrites the header with the frame count actually written, which is only
    // known once the source has been decoded to the end.
    bool close(std::string& error);

    std::uint32_t frames_written() const { return frames_written_; }

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    CacheHeader header_;
    std::uint64_t frame_bytes_ = 0;
    std::uint32_t frames_written_ = 0;
};

// Reads a .svcache. Random access is arithmetic, so seeking costs nothing.
class CacheReader {
public:
    bool open(const std::string& path, std::string& error);
    void close();
    bool is_open() const;

    const CacheHeader& header() const { return header_; }
    const std::vector<std::uint8_t>& metadata() const { return metadata_; }
    std::uint64_t frame_bytes() const { return frame_bytes_; }

    // Reads one frame into `out`, resizing it. Out-of-range indices fail rather
    // than clamping: a caller asking for a frame that is not there has a bug.
    bool read_frame(std::uint32_t index, std::vector<std::uint8_t>& out,
                    std::string& error);

    // Reads a contiguous run in one go — how the VRAM window refills itself.
    bool read_range(std::uint32_t first, std::uint32_t count,
                    std::vector<std::uint8_t>& out, std::string& error);

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    CacheHeader header_;
    std::vector<std::uint8_t> metadata_;
    std::uint64_t frame_bytes_ = 0;
    std::uint64_t data_offset_ = 0;
};

}  // namespace svj
