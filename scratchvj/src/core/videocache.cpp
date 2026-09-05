#include "core/videocache.h"

#include <algorithm>
#include <fstream>

#include "core/bytes.h"

namespace svj {
namespace {

std::uint64_t bytes_per_block(BlockFormat format) {
    return format == BlockFormat::BC1 ? 8u : 16u;
}

std::vector<std::uint8_t> encode_header(const CacheHeader& h, std::uint32_t metadata_length) {
    std::vector<std::uint8_t> out;
    out.reserve(kCacheHeaderBytes);
    put_u32(out, kCacheMagic);
    put_u16(out, kCacheVersion);
    put_u16(out, h.flags);
    put_u32(out, h.width);
    put_u32(out, h.height);
    put_u32(out, h.frame_count);
    put_u32(out, h.fps_num);
    put_u32(out, h.fps_den);
    put_u8(out, static_cast<std::uint8_t>(h.format));
    put_u8(out, 0);
    put_u16(out, 0);
    put_u32(out, metadata_length);
    out.resize(kCacheHeaderBytes, 0);  // reserved tail, zero filled
    return out;
}

}  // namespace

double CacheHeader::frame_duration_s() const {
    if (fps_num == 0) return 0.0;
    return static_cast<double>(fps_den) / static_cast<double>(fps_num);
}

double CacheHeader::duration_s() const {
    return static_cast<double>(frame_count) * frame_duration_s();
}

std::uint32_t CacheHeader::frame_at(double seconds) const {
    if (frame_count == 0 || fps_num == 0 || fps_den == 0) return 0;
    if (seconds <= 0.0) return 0;

    // Multiply by the rate rather than divide by a rounded frame duration, and
    // absorb the last bit of representation error. At 29.97 fps the naive form
    // turns a time worth exactly N frames into N - 1 often enough to land a cue
    // point one frame early, intermittently -- the worst kind of bug to chase.
    const double index =
        seconds * static_cast<double>(fps_num) / static_cast<double>(fps_den) + 1e-9;

    if (index >= static_cast<double>(frame_count - 1)) return frame_count - 1;
    return static_cast<std::uint32_t>(index);
}

double CacheHeader::time_of(std::uint32_t frame) const {
    return static_cast<double>(frame) * frame_duration_s();
}

std::uint64_t block_bytes_per_frame(std::uint32_t width, std::uint32_t height,
                                    BlockFormat format) {
    const std::uint64_t blocks_x = (static_cast<std::uint64_t>(width) + 3) / 4;
    const std::uint64_t blocks_y = (static_cast<std::uint64_t>(height) + 3) / 4;
    return blocks_x * blocks_y * bytes_per_block(format);
}

// ---------------------------------------------------------------- writer ----

struct CacheWriter::Impl {
    std::ofstream file;
    std::uint32_t metadata_length = 0;
};

bool CacheWriter::open(const std::string& path, const CacheHeader& header,
                       const std::vector<std::uint8_t>& metadata, std::string& error) {
    if (header.width == 0 || header.height == 0) {
        error = "clip dimensions must be non-zero";
        return false;
    }
    if (header.fps_num == 0 || header.fps_den == 0) {
        error = "frame rate must be non-zero";
        return false;
    }
    if (metadata.size() > 0xFFFFFFFFull) {
        error = "metadata block is too large";
        return false;
    }

    impl_ = std::make_shared<Impl>();
    impl_->file.open(path, std::ios::binary | std::ios::trunc);
    if (!impl_->file) {
        error = "cannot open '" + path + "' for writing";
        return false;
    }

    header_ = header;
    header_.frame_count = 0;  // filled in by close()
    frame_bytes_ = block_bytes_per_frame(header.width, header.height, header.format);
    frames_written_ = 0;
    impl_->metadata_length = static_cast<std::uint32_t>(metadata.size());

    const auto head = encode_header(header_, impl_->metadata_length);
    impl_->file.write(reinterpret_cast<const char*>(head.data()),
                      static_cast<std::streamsize>(head.size()));
    if (!metadata.empty()) {
        impl_->file.write(reinterpret_cast<const char*>(metadata.data()),
                          static_cast<std::streamsize>(metadata.size()));
    }
    if (!impl_->file) {
        error = "failed while writing the header of '" + path + "'";
        return false;
    }
    return true;
}

bool CacheWriter::write_frame(const std::uint8_t* data, std::size_t size, std::string& error) {
    if (!impl_ || !impl_->file) {
        error = "no cache file is open";
        return false;
    }
    // Enforced rather than trusted: a short frame would silently shift every
    // later frame, and the corruption would only show up as a wrong picture.
    if (size != frame_bytes_) {
        error = "frame is " + std::to_string(size) + " bytes, expected " +
                std::to_string(frame_bytes_);
        return false;
    }
    if (frames_written_ == 0xFFFFFFFFu) {
        error = "too many frames";
        return false;
    }
    impl_->file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!impl_->file) {
        error = "failed while writing frame " + std::to_string(frames_written_);
        return false;
    }
    ++frames_written_;
    return true;
}

bool CacheWriter::close(std::string& error) {
    if (!impl_) return true;
    if (!impl_->file) {
        error = "cache file is not writable";
        return false;
    }

    header_.frame_count = frames_written_;
    const auto head = encode_header(header_, impl_->metadata_length);
    impl_->file.seekp(0, std::ios::beg);
    impl_->file.write(reinterpret_cast<const char*>(head.data()),
                      static_cast<std::streamsize>(head.size()));
    impl_->file.flush();
    const bool ok = static_cast<bool>(impl_->file);
    impl_->file.close();
    impl_.reset();
    if (!ok) {
        error = "failed while finalising the cache header";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- reader ----

struct CacheReader::Impl {
    std::ifstream file;
};

bool CacheReader::open(const std::string& path, std::string& error) {
    auto impl = std::make_shared<Impl>();
    impl->file.open(path, std::ios::binary);
    if (!impl->file) {
        error = "cannot open '" + path + "' for reading";
        return false;
    }

    std::uint8_t head[kCacheHeaderBytes] = {};
    impl->file.read(reinterpret_cast<char*>(head), kCacheHeaderBytes);
    if (impl->file.gcount() != static_cast<std::streamsize>(kCacheHeaderBytes)) {
        error = path + ": too short to be a cache file";
        return false;
    }

    ByteReader reader(head, kCacheHeaderBytes);
    if (reader.u32() != kCacheMagic) {
        error = path + ": not a scratchvj cache file";
        return false;
    }
    const std::uint16_t version = reader.u16();
    if (version != kCacheVersion) {
        error = path + ": cache version " + std::to_string(version) + " is not supported";
        return false;
    }

    CacheHeader header;
    header.flags = reader.u16();
    header.width = reader.u32();
    header.height = reader.u32();
    header.frame_count = reader.u32();
    header.fps_num = reader.u32();
    header.fps_den = reader.u32();
    const std::uint8_t format = reader.u8();
    reader.skip(3);
    const std::uint32_t metadata_length = reader.u32();
    if (!reader.ok()) {
        error = path + ": malformed header";
        return false;
    }

    if (format < 1 || format > 3) {
        error = path + ": unknown block format " + std::to_string(format);
        return false;
    }
    header.format = static_cast<BlockFormat>(format);

    if (header.width == 0 || header.height == 0 || header.fps_num == 0 || header.fps_den == 0) {
        error = path + ": header describes an impossible clip";
        return false;
    }

    std::vector<std::uint8_t> metadata(metadata_length);
    if (metadata_length > 0) {
        impl->file.read(reinterpret_cast<char*>(metadata.data()), metadata_length);
        if (impl->file.gcount() != static_cast<std::streamsize>(metadata_length)) {
            error = path + ": metadata block is truncated";
            return false;
        }
    }

    const std::uint64_t frame_bytes =
        block_bytes_per_frame(header.width, header.height, header.format);
    const std::uint64_t data_offset = kCacheHeaderBytes + metadata_length;

    // The frame count and the file size must agree. A mismatch means a truncated
    // or interrupted analysis pass, and playing it would show garbage.
    impl->file.seekg(0, std::ios::end);
    const std::uint64_t size = static_cast<std::uint64_t>(impl->file.tellg());
    const std::uint64_t expected = data_offset + frame_bytes * header.frame_count;
    if (size < expected) {
        error = path + ": file holds " + std::to_string(size) + " bytes but its header claims " +
                std::to_string(expected) + " -- analysis was interrupted";
        return false;
    }

    impl_ = std::move(impl);
    header_ = header;
    metadata_ = std::move(metadata);
    frame_bytes_ = frame_bytes;
    data_offset_ = data_offset;
    return true;
}

void CacheReader::close() { impl_.reset(); }

bool CacheReader::is_open() const { return impl_ != nullptr; }

bool CacheReader::read_range(std::uint32_t first, std::uint32_t count,
                             std::vector<std::uint8_t>& out, std::string& error) {
    if (!impl_) {
        error = "no cache file is open";
        return false;
    }
    if (count == 0) {
        out.clear();
        return true;
    }
    // Out of range fails rather than clamps: asking for a frame that is not there
    // is a caller bug, and silently returning a different one hides it.
    if (first >= header_.frame_count ||
        static_cast<std::uint64_t>(first) + count > header_.frame_count) {
        error = "frames " + std::to_string(first) + ".." + std::to_string(first + count - 1) +
                " are outside a clip of " + std::to_string(header_.frame_count);
        return false;
    }

    const std::uint64_t offset = data_offset_ + frame_bytes_ * first;
    const std::uint64_t length = frame_bytes_ * count;
    out.resize(static_cast<std::size_t>(length));

    impl_->file.clear();
    impl_->file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    impl_->file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    if (impl_->file.gcount() != static_cast<std::streamsize>(length)) {
        error = "short read at frame " + std::to_string(first);
        return false;
    }
    return true;
}

bool CacheReader::read_frame(std::uint32_t index, std::vector<std::uint8_t>& out,
                             std::string& error) {
    return read_range(index, 1, out, error);
}

}  // namespace svj
