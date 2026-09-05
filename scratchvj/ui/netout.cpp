#include "netout.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace svj::ui {

struct ControlStream::Impl {
#ifdef _WIN32
    SOCKET socket_handle = INVALID_SOCKET;
    bool winsock_up = false;
#else
    int socket_handle = -1;
#endif
    sockaddr_in target{};
};

ControlStream::ControlStream() : impl_(new Impl) {}

ControlStream::~ControlStream() {
    close();
    delete impl_;
}

bool ControlStream::open(const std::string& host, std::uint16_t port) {
    close();
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    impl_->winsock_up = true;
    impl_->socket_handle = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (impl_->socket_handle == INVALID_SOCKET) return false;
#else
    impl_->socket_handle = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (impl_->socket_handle < 0) return false;
#endif

    impl_->target = sockaddr_in{};
    impl_->target.sin_family = AF_INET;
    impl_->target.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &impl_->target.sin_addr) != 1) return false;

    active_ = true;
    return true;
}

void ControlStream::close() {
#ifdef _WIN32
    if (impl_->socket_handle != INVALID_SOCKET) {
        ::closesocket(impl_->socket_handle);
        impl_->socket_handle = INVALID_SOCKET;
    }
    if (impl_->winsock_up) {
        WSACleanup();
        impl_->winsock_up = false;
    }
#else
    if (impl_->socket_handle >= 0) {
        ::close(impl_->socket_handle);
        impl_->socket_handle = -1;
    }
#endif
    active_ = false;
}

void ControlStream::send_bytes(const std::uint8_t* data, std::size_t size) {
    if (!active_) return;
    // Fire and forget. A dropped datagram costs one frame of state out of
    // hundreds per second; blocking a render loop on the network would cost
    // more. Nothing here checks the return value on purpose.
    ::sendto(impl_->socket_handle, reinterpret_cast<const char*>(data),
             static_cast<int>(size), 0, reinterpret_cast<sockaddr*>(&impl_->target),
             sizeof(impl_->target));
}

void ControlStream::send_state(const StatePacket& packet) {
    encode_state(packet, scratch_);
    send_bytes(scratch_.data(), scratch_.size());
}

void ControlStream::send_schema(const SchemaPacket& packet) {
    encode_schema(packet, scratch_);
    send_bytes(scratch_.data(), scratch_.size());
}

}  // namespace svj::ui
