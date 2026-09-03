// scratchvj — the wire format carrying surface and platter state to Unreal.
//
// Deliberately NOT the engine's native OSC plugin: that allocates UObjects per
// message and dispatches on the game thread, which at ~375 Hz is garbage for
// nothing. This is a fixed-layout little-endian payload read off a UDP socket on a
// dedicated thread. An OSC mirror runs alongside for TouchDesigner and Resolume.
//
// Two packet kinds. State packets stream at audio-block rate and carry only
// values. Schema packets are sent rarely -- at startup, and whenever the control
// set changes -- and name those values. The receiver caches the schema and matches
// it to state packets by `schema_hash`, so the high-rate path never carries strings.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/surface.h"

namespace svj {

inline constexpr std::uint32_t kProtocolMagic = 0x314A5653;  // "SVJ1" little-endian
inline constexpr std::uint16_t kProtocolVersion = 1;

enum class PacketKind : std::uint16_t {
    State = 1,
    Schema = 2,
};

// Momentary events, as a bitfield so a whole frame's gestures fit in one word.
enum GestureBit : std::uint32_t {
    kGestureScratchingA = 1u << 0,
    kGestureScratchingB = 1u << 1,
    kGestureBackspinA = 1u << 2,
    kGestureBackspinB = 1u << 3,
    kGestureHoldingA = 1u << 4,  // timecode confidence lost, output frozen
    kGestureHoldingB = 1u << 5,
    kGestureTransform = 1u << 6,  // fast crossfader cuts with the platter running
};

struct DeckWire {
    float pos_s = 0.0f;         // position on the control record, seconds
    float velocity = 0.0f;      // signed speed ratio, 1.0 nominal
    float acceleration = 0.0f;
    float scratch_rate = 0.0f;  // direction reversals per second
    float confidence = 0.0f;    // timecode lock quality, 0..1
    float anchor_s = 0.0f;      // follower mode: timecode-to-clip offset
    float drift_s = 0.0f;       // follower mode: measured drift since anchoring
};

struct StatePacket {
    std::uint64_t t_us = 0;         // monotonic clock of the sender
    std::uint32_t schema_hash = 0;  // identifies the control layout of `values`
    DeckWire deck_a;
    DeckWire deck_b;
    std::uint32_t gesture_bits = 0;
    std::vector<float> values;  // one per control, in schema order
    std::vector<bool> known;    // parallel to `values`; false means never touched
};

struct SchemaEntry {
    std::string id;
    ControlKind kind = ControlKind::Knob;
};

struct SchemaPacket {
    std::uint32_t schema_hash = 0;
    std::vector<SchemaEntry> entries;
};

// Order-sensitive hash of the control layout. Two senders agreeing on this value
// agree on what every float in a state packet means.
std::uint32_t schema_hash(const std::vector<SchemaEntry>& entries);

std::vector<SchemaEntry> schema_from(const Surface& surface);

void encode_state(const StatePacket& packet, std::vector<std::uint8_t>& out);
void encode_schema(const SchemaPacket& packet, std::vector<std::uint8_t>& out);

// Returns the packet kind on success. Peeking first lets a receiver route a
// datagram without decoding it twice.
bool peek_kind(const std::uint8_t* data, std::size_t size, PacketKind& kind);

bool decode_state(const std::uint8_t* data, std::size_t size, StatePacket& out);
bool decode_schema(const std::uint8_t* data, std::size_t size, SchemaPacket& out);

}  // namespace svj
