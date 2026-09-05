// scratchvj — explicit little-endian byte packing.
//
// Everything this project writes to a socket or to disk is packed byte by byte
// rather than by casting a struct: no padding surprises, no alignment traps, and
// the same bytes from every compiler on every platform.
#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace svj {

static_assert(std::numeric_limits<float>::is_iec559,
              "the wire and file formats assume IEEE 754 floats");

inline void put_u8(std::vector<std::uint8_t>& out, std::uint8_t v) { out.push_back(v); }

inline void put_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

inline void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}

inline void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}

inline void put_f32(std::vector<std::uint8_t>& out, float v) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    put_u32(out, bits);
}

// Bounds-checked reader. Any short read leaves `ok` false and every subsequent
// call is a no-op, so callers check once at the end instead of at every field.
class ByteReader {
public:
    ByteReader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}

    std::uint8_t u8() {
        if (!take(1)) return 0;
        return data_[at_ - 1];
    }

    std::uint16_t u16() {
        if (!take(2)) return 0;
        return static_cast<std::uint16_t>(data_[at_ - 2] | (data_[at_ - 1] << 8));
    }

    std::uint32_t u32() {
        if (!take(4)) return 0;
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(data_[at_ - 4 + i]) << (8 * i);
        return v;
    }

    std::uint64_t u64() {
        if (!take(8)) return 0;
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(data_[at_ - 8 + i]) << (8 * i);
        return v;
    }

    float f32() {
        const std::uint32_t bits = u32();
        float v = 0.0f;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    std::string str(std::size_t length) {
        if (!take(length)) return {};
        return std::string(reinterpret_cast<const char*>(data_ + at_ - length), length);
    }

    void skip(std::size_t n) { take(n); }

    bool ok() const { return ok_; }
    std::size_t remaining() const { return ok_ ? size_ - at_ : 0; }
    std::size_t position() const { return at_; }

private:
    bool take(std::size_t n) {
        if (!ok_ || size_ - at_ < n) {
            ok_ = false;
            return false;
        }
        at_ += n;
        return true;
    }

    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t at_ = 0;
    bool ok_ = true;
};

}  // namespace svj
