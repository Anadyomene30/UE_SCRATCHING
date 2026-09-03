#include "core/protocol.h"
#include "harness.h"

using namespace svj;

namespace {

StatePacket sample_state() {
    StatePacket p;
    p.t_us = 1234567890123ULL;
    p.schema_hash = 0xDEADBEEF;
    p.deck_a = DeckWire{83.4f, -1.8f, -12.5f, 6.4f, 0.98f, 0.0f, 0.02f};
    p.deck_b = DeckWire{7.4f, 1.0f, 0.0f, 0.0f, 0.96f, 0.0f, 0.0f};
    p.gesture_bits = kGestureScratchingA | kGestureBackspinA;
    p.values = {0.0f, 0.25f, 0.5f, 0.74f, 1.0f, 0.1f, 0.2f, 0.3f, 0.4f};
    p.known = {true, false, true, true, false, false, true, true, true};
    return p;
}

}  // namespace

SVJ_TEST("protocol: a state packet survives a round trip") {
    const StatePacket original = sample_state();
    std::vector<std::uint8_t> bytes;
    encode_state(original, bytes);

    StatePacket decoded;
    CHECK(decode_state(bytes.data(), bytes.size(), decoded));
    CHECK_EQ(decoded.t_us, original.t_us);
    CHECK_EQ(decoded.schema_hash, original.schema_hash);
    CHECK_EQ(decoded.gesture_bits, original.gesture_bits);
    CHECK_NEAR(decoded.deck_a.velocity, -1.8, 1e-6);
    CHECK_NEAR(decoded.deck_a.scratch_rate, 6.4, 1e-6);
    CHECK_NEAR(decoded.deck_b.confidence, 0.96, 1e-6);
    CHECK_EQ(decoded.values.size(), original.values.size());
    for (std::size_t i = 0; i < original.values.size(); ++i) {
        CHECK_NEAR(decoded.values[i], original.values[i], 1e-6);
        CHECK_EQ(bool(decoded.known[i]), bool(original.known[i]));
    }
}

SVJ_TEST("protocol: the known bitset spans more than one byte correctly") {
    StatePacket p;
    p.values.assign(20, 0.0f);
    p.known.assign(20, false);
    p.known[0] = true;
    p.known[7] = true;
    p.known[8] = true;
    p.known[19] = true;

    std::vector<std::uint8_t> bytes;
    encode_state(p, bytes);
    StatePacket decoded;
    CHECK(decode_state(bytes.data(), bytes.size(), decoded));
    for (std::size_t i = 0; i < 20; ++i) {
        const bool expected = (i == 0 || i == 7 || i == 8 || i == 19);
        CHECK_EQ(bool(decoded.known[i]), expected);
    }
}

SVJ_TEST("protocol: an empty control list round trips") {
    StatePacket p;
    std::vector<std::uint8_t> bytes;
    encode_state(p, bytes);
    StatePacket decoded;
    CHECK(decode_state(bytes.data(), bytes.size(), decoded));
    CHECK(decoded.values.empty());
}

SVJ_TEST("protocol: a truncated packet is rejected at every length") {
    std::vector<std::uint8_t> bytes;
    encode_state(sample_state(), bytes);
    for (std::size_t length = 0; length < bytes.size(); ++length) {
        StatePacket decoded;
        CHECK(!decode_state(bytes.data(), length, decoded));
    }
    StatePacket decoded;
    CHECK(decode_state(bytes.data(), bytes.size(), decoded));
}

SVJ_TEST("protocol: a bad magic or version is rejected") {
    std::vector<std::uint8_t> bytes;
    encode_state(sample_state(), bytes);

    auto corrupted = bytes;
    corrupted[0] ^= 0xFF;
    StatePacket decoded;
    CHECK(!decode_state(corrupted.data(), corrupted.size(), decoded));

    corrupted = bytes;
    corrupted[4] = 0x7F;  // version
    CHECK(!decode_state(corrupted.data(), corrupted.size(), decoded));
}

SVJ_TEST("protocol: decoding refuses a packet whose control count exceeds its payload") {
    // A hostile or corrupted length field must not drive a huge allocation.
    std::vector<std::uint8_t> bytes;
    StatePacket small;
    small.values.assign(2, 0.5f);
    small.known.assign(2, true);
    encode_state(small, bytes);

    // Overwrite the count with something the datagram cannot possibly hold.
    const std::size_t count_offset = 4 + 2 + 2 + 8 + 4 + (7 * 4) * 2 + 4;
    bytes[count_offset] = 0xFF;
    bytes[count_offset + 1] = 0xFF;

    StatePacket decoded;
    CHECK(!decode_state(bytes.data(), bytes.size(), decoded));
}

SVJ_TEST("protocol: a schema packet survives a round trip") {
    SchemaPacket schema;
    schema.entries = {
        SchemaEntry{"xfader", ControlKind::Fader},
        SchemaEntry{"ch1.eq.hi", ControlKind::Knob},
        SchemaEntry{"pad.elite.a.1", ControlKind::Pad},
    };
    schema.schema_hash = schema_hash(schema.entries);

    std::vector<std::uint8_t> bytes;
    encode_schema(schema, bytes);
    SchemaPacket decoded;
    CHECK(decode_schema(bytes.data(), bytes.size(), decoded));
    CHECK_EQ(decoded.schema_hash, schema.schema_hash);
    CHECK_EQ(decoded.entries.size(), std::size_t{3});
    CHECK_EQ(decoded.entries[1].id, std::string("ch1.eq.hi"));
    CHECK(decoded.entries[2].kind == ControlKind::Pad);
}

SVJ_TEST("protocol: the schema hash depends on order") {
    const std::vector<SchemaEntry> a{SchemaEntry{"one", ControlKind::Knob},
                                     SchemaEntry{"two", ControlKind::Knob}};
    const std::vector<SchemaEntry> b{SchemaEntry{"two", ControlKind::Knob},
                                     SchemaEntry{"one", ControlKind::Knob}};
    CHECK(schema_hash(a) != schema_hash(b));
}

SVJ_TEST("protocol: the schema hash distinguishes ids that concatenate alike") {
    const std::vector<SchemaEntry> a{SchemaEntry{"ab", ControlKind::Knob},
                                     SchemaEntry{"c", ControlKind::Knob}};
    const std::vector<SchemaEntry> b{SchemaEntry{"a", ControlKind::Knob},
                                     SchemaEntry{"bc", ControlKind::Knob}};
    CHECK(schema_hash(a) != schema_hash(b));
}

SVJ_TEST("protocol: the schema hash notices a changed control kind") {
    const std::vector<SchemaEntry> a{SchemaEntry{"x", ControlKind::Knob}};
    const std::vector<SchemaEntry> b{SchemaEntry{"x", ControlKind::Fader}};
    CHECK(schema_hash(a) != schema_hash(b));
}

SVJ_TEST("protocol: a schema built from a surface matches its declaration order") {
    Surface surface;
    surface.declare("xfader", ControlKind::Fader);
    surface.declare("ch1.eq.hi", ControlKind::Knob);
    const auto entries = schema_from(surface);
    CHECK_EQ(entries.size(), std::size_t{2});
    CHECK_EQ(entries[0].id, std::string("xfader"));
    CHECK(entries[1].kind == ControlKind::Knob);
}

SVJ_TEST("protocol: peeking reports the kind without a full decode") {
    std::vector<std::uint8_t> bytes;
    encode_state(sample_state(), bytes);
    PacketKind kind{};
    CHECK(peek_kind(bytes.data(), bytes.size(), kind));
    CHECK(kind == PacketKind::State);

    SchemaPacket schema;
    encode_schema(schema, bytes);
    CHECK(peek_kind(bytes.data(), bytes.size(), kind));
    CHECK(kind == PacketKind::Schema);
}

SVJ_TEST("protocol: decoders reject the other packet kind") {
    std::vector<std::uint8_t> bytes;
    encode_state(sample_state(), bytes);
    SchemaPacket schema;
    CHECK(!decode_schema(bytes.data(), bytes.size(), schema));
}
