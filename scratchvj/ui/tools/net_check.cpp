// scratchvj — proves the UDP control stream end to end, from outside the app.
//
// Binds the control port, listens for a few seconds, and decodes what arrives
// with core/protocol -- the same decoder Unreal's plugin mirrors. It checks the
// two things a client actually depends on: that a schema turns up on its own
// (a late joiner must not stay deaf), and that state packets reference it by
// hash. Exit 0 when both held.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <vector>

#include "core/protocol.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(7331);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::fprintf(stderr, "port 7331 occupe\n");
        return 1;
    }
    DWORD timeout_ms = 6000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms),
               sizeof(timeout_ms));

    std::vector<std::uint8_t> buffer(65536);
    svj::SchemaPacket schema;
    bool have_schema = false;
    unsigned long long states = 0, matched = 0;
    svj::StatePacket last;

    for (int i = 0; i < 4000; ++i) {
        const int received =
            recv(sock, reinterpret_cast<char*>(buffer.data()),
                 static_cast<int>(buffer.size()), 0);
        if (received <= 0) break;  // timeout: the sender has gone quiet

        svj::PacketKind kind;
        if (!svj::peek_kind(buffer.data(), static_cast<std::size_t>(received), kind)) {
            continue;
        }
        if (kind == svj::PacketKind::Schema) {
            have_schema =
                svj::decode_schema(buffer.data(), static_cast<std::size_t>(received), schema);
        } else if (kind == svj::PacketKind::State) {
            if (svj::decode_state(buffer.data(), static_cast<std::size_t>(received), last)) {
                ++states;
                if (have_schema && last.schema_hash == schema.schema_hash) ++matched;
            }
        }
        if (states >= 300 && have_schema) break;  // seen plenty; verdict is in
    }
    closesocket(sock);
    WSACleanup();

    if (states == 0) {
        std::fprintf(stderr, "aucun paquet d'etat recu sur 7331\n");
        return 1;
    }
    if (!have_schema) {
        std::fprintf(stderr, "%llu etats mais aucun schema : un client tardif resterait sourd\n",
                     states);
        return 2;
    }
    std::printf("schema %u (%zu controles)  %llu etats recus, %llu au bon hash\n",
                schema.schema_hash, schema.entries.size(), states, matched);
    std::printf("deck A pos %.2fs vel %.2f  deck B pos %.2fs vel %.2f  gestes %08x\n",
                last.deck_a.pos_s, last.deck_a.velocity, last.deck_b.pos_s,
                last.deck_b.velocity, last.gesture_bits);
    return matched > 0 ? 0 : 2;
}
