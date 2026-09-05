// scratchvj — the UDP control stream, finally leaving the machine.
//
// core/protocol has encoded these packets since the first commit; this is the
// socket underneath. It lives in ui/ and not core/ for the same reason the
// window does: core stays free of every platform surface, sockets included, so
// its tests never need a network stack.
//
// The stream is fire-and-forget datagrams: state every frame, schema once a
// second. Re-sending the schema on a timer rather than once at startup is what
// lets a client attach mid-set and still learn what the floats mean -- the
// alternative is a client that joins late and stays deaf until a control is
// added.
#pragma once

#include <cstdint>
#include <string>

#include "core/protocol.h"

namespace svj::ui {

inline constexpr std::uint16_t kDefaultControlPort = 7331;

class ControlStream {
public:
    ControlStream();
    ~ControlStream();

    ControlStream(const ControlStream&) = delete;
    ControlStream& operator=(const ControlStream&) = delete;

    // Opens the socket towards host:port. Returns false when the network stack
    // refuses, which callers treat as "stream disabled", not as fatal.
    bool open(const std::string& host, std::uint16_t port);
    void close();
    bool active() const { return active_; }

    void send_state(const StatePacket& packet);
    void send_schema(const SchemaPacket& packet);

private:
    void send_bytes(const std::uint8_t* data, std::size_t size);

    struct Impl;
    Impl* impl_ = nullptr;
    bool active_ = false;
    std::vector<std::uint8_t> scratch_;
};

}  // namespace svj::ui
