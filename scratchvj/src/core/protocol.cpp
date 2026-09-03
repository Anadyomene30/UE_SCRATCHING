#include "core/protocol.h"

#include <cstring>
#include <limits>

namespace svj {
namespace {

static_assert(std::numeric_limits<float>::is_iec559,
              "the wire format assumes IEEE 754 floats");

void put_u8(std::vector<std::uint8_t>& out, std::uint8_t v) { out.push_back(v); }

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}

void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
}

void put_f32(std::vector<std::uint8_t>& out, float v) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    put_u32(out, bits);
}

void put_deck(std::vector<std::uint8_t>& out, const DeckWire& d) {
    put_f32(out, d.pos_s);
    put_f32(out, d.velocity);
    put_f32(out, d.acceleration);
    put_f32(out, d.scratch_rate);
    put_f32(out, d.confidence);
    put_f32(out, d.anchor_s);
    put_f32(out, d.drift_s);
}

// Bounds-checked little-endian reader. Any short read leaves `ok` false and every
// subsequent call is a no-op, so callers check once at the end.
class Reader {
public:
    Reader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}

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

    bool ok() const { return ok_; }
    std::size_t remaining() const { return ok_ ? size_ - at_ : 0; }

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

DeckWire read_deck(Reader& r) {
    DeckWire d;
    d.pos_s = r.f32();
    d.velocity = r.f32();
    d.acceleration = r.f32();
    d.scratch_rate = r.f32();
    d.confidence = r.f32();
    d.anchor_s = r.f32();
    d.drift_s = r.f32();
    return d;
}

void put_header(std::vector<std::uint8_t>& out, PacketKind kind) {
    put_u32(out, kProtocolMagic);
    put_u16(out, kProtocolVersion);
    put_u16(out, static_cast<std::uint16_t>(kind));
}

bool read_header(Reader& r, PacketKind& kind) {
    if (r.u32() != kProtocolMagic) return false;
    if (r.u16() != kProtocolVersion) return false;
    kind = static_cast<PacketKind>(r.u16());
    return r.ok();
}

}  // namespace

std::uint32_t schema_hash(const std::vector<SchemaEntry>& entries) {
    // FNV-1a over the ordered ids and kinds. Order matters: it is what makes the
    // hash a promise about the meaning of each slot in a state packet.
    std::uint32_t h = 2166136261u;
    const auto mix = [&h](std::uint8_t byte) {
        h ^= byte;
        h *= 16777619u;
    };
    for (const SchemaEntry& e : entries) {
        for (const char c : e.id) mix(static_cast<std::uint8_t>(c));
        mix(static_cast<std::uint8_t>(e.kind));
        mix(0x1F);  // separator, so {"ab","c"} and {"a","bc"} differ
    }
    return h;
}

std::vector<SchemaEntry> schema_from(const Surface& surface) {
    std::vector<SchemaEntry> entries;
    entries.reserve(surface.size());
    for (std::size_t i = 0; i < surface.size(); ++i) {
        const Control& c = surface.at(static_cast<ControlIndex>(i));
        entries.push_back(SchemaEntry{c.id, c.kind});
    }
    return entries;
}

void encode_state(const StatePacket& packet, std::vector<std::uint8_t>& out) {
    out.clear();
    put_header(out, PacketKind::State);
    put_u64(out, packet.t_us);
    put_u32(out, packet.schema_hash);
    put_deck(out, packet.deck_a);
    put_deck(out, packet.deck_b);
    put_u32(out, packet.gesture_bits);

    const auto count = static_cast<std::uint16_t>(packet.values.size());
    put_u16(out, count);
    for (const float v : packet.values) put_f32(out, v);

    // Known flags as a bitset: one byte per eight controls.
    const std::size_t bytes = (packet.values.size() + 7) / 8;
    for (std::size_t byte = 0; byte < bytes; ++byte) {
        std::uint8_t bits = 0;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            const std::size_t index = byte * 8 + bit;
            if (index < packet.known.size() && packet.known[index]) {
                bits |= static_cast<std::uint8_t>(1u << bit);
            }
        }
        put_u8(out, bits);
    }
}

void encode_schema(const SchemaPacket& packet, std::vector<std::uint8_t>& out) {
    out.clear();
    put_header(out, PacketKind::Schema);
    put_u32(out, packet.schema_hash);
    put_u16(out, static_cast<std::uint16_t>(packet.entries.size()));
    for (const SchemaEntry& e : packet.entries) {
        put_u8(out, static_cast<std::uint8_t>(e.kind));
        const auto length = static_cast<std::uint8_t>(e.id.size() > 255 ? 255 : e.id.size());
        put_u8(out, length);
        out.insert(out.end(), e.id.begin(), e.id.begin() + length);
    }
}

bool peek_kind(const std::uint8_t* data, std::size_t size, PacketKind& kind) {
    Reader r(data, size);
    return read_header(r, kind);
}

bool decode_state(const std::uint8_t* data, std::size_t size, StatePacket& out) {
    Reader r(data, size);
    PacketKind kind{};
    if (!read_header(r, kind) || kind != PacketKind::State) return false;

    out.t_us = r.u64();
    out.schema_hash = r.u32();
    out.deck_a = read_deck(r);
    out.deck_b = read_deck(r);
    out.gesture_bits = r.u32();

    const std::uint16_t count = r.u16();
    if (!r.ok()) return false;
    // Reject a length that the datagram cannot possibly satisfy before reserving.
    if (r.remaining() < static_cast<std::size_t>(count) * 4) return false;

    out.values.assign(count, 0.0f);
    for (std::uint16_t i = 0; i < count; ++i) out.values[i] = r.f32();

    out.known.assign(count, false);
    const std::size_t bytes = (static_cast<std::size_t>(count) + 7) / 8;
    for (std::size_t byte = 0; byte < bytes; ++byte) {
        const std::uint8_t bits = r.u8();
        for (std::size_t bit = 0; bit < 8; ++bit) {
            const std::size_t index = byte * 8 + bit;
            if (index < out.known.size()) out.known[index] = (bits >> bit) & 1u;
        }
    }

    return r.ok();
}

bool decode_schema(const std::uint8_t* data, std::size_t size, SchemaPacket& out) {
    Reader r(data, size);
    PacketKind kind{};
    if (!read_header(r, kind) || kind != PacketKind::Schema) return false;

    out.schema_hash = r.u32();
    const std::uint16_t count = r.u16();
    if (!r.ok()) return false;

    out.entries.clear();
    out.entries.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        SchemaEntry e;
        e.kind = static_cast<ControlKind>(r.u8());
        const std::uint8_t length = r.u8();
        e.id = r.str(length);
        if (!r.ok()) return false;
        out.entries.push_back(std::move(e));
    }

    return r.ok();
}

}  // namespace svj
