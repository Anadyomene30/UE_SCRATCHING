// scratchvj — recording and replaying a performance's control stream.
//
// A take is the timestamped stream of everything the surface and the platters did.
// It exists for two reasons, and the first is the one that matters day to day:
// DEVELOPMENT WITHOUT TURNTABLES. Record thirty seconds once, and every later
// change can be exercised against a real performance instead of a guess.
//
// The second is that a live set can be replayed exactly, to render it offline at
// a quality no live pass could reach.
//
// A record is simply a length-prefixed protocol packet, so the file format and the
// wire format cannot drift apart: whatever Unreal can receive, a take can hold.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/protocol.h"

namespace svj {

inline constexpr std::uint32_t kTakeMagic = 0x31545653;  // "SVT1" little-endian
inline constexpr std::uint16_t kTakeVersion = 1;
inline constexpr std::size_t kTakeHeaderBytes = 32;

class TakeWriter {
public:
    bool open(const std::string& path, std::string& error);
    bool write_schema(const SchemaPacket& schema, std::string& error);
    bool write_state(const StatePacket& state, std::string& error);
    bool close(std::string& error);

    std::uint32_t records_written() const { return records_; }

private:
    bool write_record(const std::vector<std::uint8_t>& payload, std::string& error);

    struct Impl;
    std::shared_ptr<Impl> impl_;
    std::uint32_t records_ = 0;
};

class TakeReader {
public:
    bool open(const std::string& path, std::string& error);
    void close();
    bool is_open() const { return impl_ != nullptr; }

    std::uint32_t record_count() const { return record_count_; }

    // Reads the next record. `kind` says which of the two outputs was filled.
    // Returns false at the end of the take or on a malformed record, with
    // `error` empty in the former case so the caller can tell them apart.
    bool next(PacketKind& kind, StatePacket& state, SchemaPacket& schema,
              std::string& error);

    void rewind();

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    std::uint32_t record_count_ = 0;
};

}  // namespace svj
