#include "core/take.h"

#include <fstream>

#include "core/bytes.h"

namespace svj {
namespace {

constexpr std::uint32_t kMaxRecordBytes = 4u * 1024u * 1024u;

std::vector<std::uint8_t> encode_take_header(std::uint32_t records) {
    std::vector<std::uint8_t> out;
    out.reserve(kTakeHeaderBytes);
    put_u32(out, kTakeMagic);
    put_u16(out, kTakeVersion);
    put_u16(out, 0);
    put_u32(out, records);
    out.resize(kTakeHeaderBytes, 0);
    return out;
}

}  // namespace

// ---------------------------------------------------------------- writer ----

struct TakeWriter::Impl {
    std::ofstream file;
};

bool TakeWriter::open(const std::string& path, std::string& error) {
    impl_ = std::make_shared<Impl>();
    impl_->file.open(path, std::ios::binary | std::ios::trunc);
    if (!impl_->file) {
        error = "cannot open '" + path + "' for writing";
        return false;
    }
    records_ = 0;
    const auto head = encode_take_header(0);
    impl_->file.write(reinterpret_cast<const char*>(head.data()),
                      static_cast<std::streamsize>(head.size()));
    return static_cast<bool>(impl_->file);
}

bool TakeWriter::write_record(const std::vector<std::uint8_t>& payload, std::string& error) {
    if (!impl_ || !impl_->file) {
        error = "no take is open";
        return false;
    }
    if (payload.size() > kMaxRecordBytes) {
        error = "record is too large";
        return false;
    }
    std::vector<std::uint8_t> length;
    put_u32(length, static_cast<std::uint32_t>(payload.size()));
    impl_->file.write(reinterpret_cast<const char*>(length.data()), 4);
    impl_->file.write(reinterpret_cast<const char*>(payload.data()),
                      static_cast<std::streamsize>(payload.size()));
    if (!impl_->file) {
        error = "failed while writing a take record";
        return false;
    }
    ++records_;
    return true;
}

bool TakeWriter::write_schema(const SchemaPacket& schema, std::string& error) {
    std::vector<std::uint8_t> bytes;
    encode_schema(schema, bytes);
    return write_record(bytes, error);
}

bool TakeWriter::write_state(const StatePacket& state, std::string& error) {
    std::vector<std::uint8_t> bytes;
    encode_state(state, bytes);
    return write_record(bytes, error);
}

bool TakeWriter::close(std::string& error) {
    if (!impl_) return true;
    if (!impl_->file) {
        error = "take file is not writable";
        return false;
    }
    const auto head = encode_take_header(records_);
    impl_->file.seekp(0, std::ios::beg);
    impl_->file.write(reinterpret_cast<const char*>(head.data()),
                      static_cast<std::streamsize>(head.size()));
    impl_->file.flush();
    const bool ok = static_cast<bool>(impl_->file);
    impl_->file.close();
    impl_.reset();
    if (!ok) {
        error = "failed while finalising the take";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- reader ----

struct TakeReader::Impl {
    std::ifstream file;
};

bool TakeReader::open(const std::string& path, std::string& error) {
    auto impl = std::make_shared<Impl>();
    impl->file.open(path, std::ios::binary);
    if (!impl->file) {
        error = "cannot open '" + path + "' for reading";
        return false;
    }

    std::uint8_t head[kTakeHeaderBytes] = {};
    impl->file.read(reinterpret_cast<char*>(head), kTakeHeaderBytes);
    if (impl->file.gcount() != static_cast<std::streamsize>(kTakeHeaderBytes)) {
        error = path + ": too short to be a take";
        return false;
    }

    ByteReader reader(head, kTakeHeaderBytes);
    if (reader.u32() != kTakeMagic) {
        error = path + ": not a scratchvj take";
        return false;
    }
    const std::uint16_t version = reader.u16();
    if (version != kTakeVersion) {
        error = path + ": take version " + std::to_string(version) + " is not supported";
        return false;
    }
    reader.u16();
    record_count_ = reader.u32();
    if (!reader.ok()) {
        error = path + ": malformed take header";
        return false;
    }

    impl_ = std::move(impl);
    return true;
}

void TakeReader::close() { impl_.reset(); }

void TakeReader::rewind() {
    if (!impl_) return;
    impl_->file.clear();
    impl_->file.seekg(static_cast<std::streamoff>(kTakeHeaderBytes), std::ios::beg);
}

bool TakeReader::next(PacketKind& kind, StatePacket& state, SchemaPacket& schema,
                      std::string& error) {
    error.clear();
    if (!impl_) {
        error = "no take is open";
        return false;
    }

    std::uint8_t length_bytes[4] = {};
    impl_->file.read(reinterpret_cast<char*>(length_bytes), 4);
    if (impl_->file.gcount() != 4) return false;  // clean end of take

    ByteReader length_reader(length_bytes, 4);
    const std::uint32_t length = length_reader.u32();
    if (length == 0 || length > kMaxRecordBytes) {
        error = "take record declares an impossible length";
        return false;
    }

    std::vector<std::uint8_t> payload(length);
    impl_->file.read(reinterpret_cast<char*>(payload.data()), length);
    if (impl_->file.gcount() != static_cast<std::streamsize>(length)) {
        error = "take ends in the middle of a record";
        return false;
    }

    if (!peek_kind(payload.data(), payload.size(), kind)) {
        error = "take record is not a recognised packet";
        return false;
    }

    switch (kind) {
        case PacketKind::State:
            if (!decode_state(payload.data(), payload.size(), state)) {
                error = "malformed state record";
                return false;
            }
            return true;
        case PacketKind::Schema:
            if (!decode_schema(payload.data(), payload.size(), schema)) {
                error = "malformed schema record";
                return false;
            }
            return true;
    }

    error = "unknown record kind";
    return false;
}

}  // namespace svj
