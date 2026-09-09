#include <cstdio>

#include "core/take.h"
#include "harness.h"

using namespace svj;

namespace {

struct TempFile {
    explicit TempFile(std::string name) : path(std::move(name)) {}
    ~TempFile() { std::remove(path.c_str()); }
    std::string path;
};

StatePacket state_at(std::uint64_t t_us, float velocity) {
    StatePacket p;
    p.t_us = t_us;
    p.schema_hash = 0xABCD1234;
    p.deck_a.velocity = velocity;
    p.deck_a.confidence = 0.98f;
    p.values = {0.5f, 0.25f};
    p.known = {true, false};
    return p;
}

}  // namespace

SVJ_TEST("take: a recorded stream replays exactly") {
    TempFile temp("svj_take_roundtrip.scratchtake");
    SchemaPacket schema;
    schema.entries = {SchemaEntry{"xfader", ControlKind::Fader},
                      SchemaEntry{"ch1.eq.hi", ControlKind::Knob}};
    schema.schema_hash = schema_hash(schema.entries);

    std::string error;
    TakeWriter writer;
    CHECK(writer.open(temp.path, error));
    CHECK(writer.write_schema(schema, error));
    for (int i = 0; i < 20; ++i) {
        CHECK(writer.write_state(state_at(static_cast<std::uint64_t>(i) * 2667,
                                          static_cast<float>(i) * 0.1f),
                                 error));
    }
    CHECK_EQ(writer.records_written(), std::uint32_t{21});
    CHECK(writer.close(error));

    TakeReader reader;
    CHECK(reader.open(temp.path, error));
    CHECK_EQ(reader.record_count(), std::uint32_t{21});

    PacketKind kind{};
    StatePacket state;
    SchemaPacket read_schema;

    CHECK(reader.next(kind, state, read_schema, error));
    CHECK(kind == PacketKind::Schema);
    CHECK_EQ(read_schema.entries.size(), std::size_t{2});
    CHECK_EQ(read_schema.entries[0].id, std::string("xfader"));

    int states = 0;
    while (reader.next(kind, state, read_schema, error)) {
        CHECK(kind == PacketKind::State);
        CHECK_EQ(state.t_us, static_cast<std::uint64_t>(states) * 2667);
        CHECK_NEAR(state.deck_a.velocity, states * 0.1, 1e-5);
        ++states;
    }
    CHECK_EQ(states, 20);
    CHECK_EQ(error, std::string());  // a clean end is not an error
}

SVJ_TEST("take: rewinding replays from the start again") {
    TempFile temp("svj_take_rewind.scratchtake");
    std::string error;
    TakeWriter writer;
    CHECK(writer.open(temp.path, error));
    for (int i = 0; i < 5; ++i) CHECK(writer.write_state(state_at(i, 1.0f), error));
    CHECK(writer.close(error));

    TakeReader reader;
    CHECK(reader.open(temp.path, error));
    PacketKind kind{};
    StatePacket state;
    SchemaPacket schema;

    int first_pass = 0;
    while (reader.next(kind, state, schema, error)) ++first_pass;

    reader.rewind();
    int second_pass = 0;
    while (reader.next(kind, state, schema, error)) ++second_pass;

    CHECK_EQ(first_pass, 5);
    CHECK_EQ(second_pass, 5);
}

SVJ_TEST("take: an empty take opens and yields nothing") {
    TempFile temp("svj_take_empty.scratchtake");
    std::string error;
    TakeWriter writer;
    CHECK(writer.open(temp.path, error));
    CHECK(writer.close(error));

    TakeReader reader;
    CHECK(reader.open(temp.path, error));
    CHECK_EQ(reader.record_count(), std::uint32_t{0});

    PacketKind kind{};
    StatePacket state;
    SchemaPacket schema;
    CHECK(!reader.next(kind, state, schema, error));
    CHECK(error.empty());
}

SVJ_TEST("take: a take cut off mid-record is reported, not silently truncated") {
    TempFile temp("svj_take_cut.scratchtake");
    std::string error;
    TakeWriter writer;
    CHECK(writer.open(temp.path, error));
    for (int i = 0; i < 4; ++i) CHECK(writer.write_state(state_at(i, 1.0f), error));
    CHECK(writer.close(error));

    {
        std::FILE* f = std::fopen(temp.path.c_str(), "rb");
        std::vector<std::uint8_t> all;
        std::uint8_t buffer[4096];
        std::size_t got = 0;
        while ((got = std::fread(buffer, 1, sizeof(buffer), f)) > 0) {
            all.insert(all.end(), buffer, buffer + got);
        }
        std::fclose(f);
        all.resize(all.size() - 20);  // lop off part of the last record
        std::FILE* out = std::fopen(temp.path.c_str(), "wb");
        std::fwrite(all.data(), 1, all.size(), out);
        std::fclose(out);
    }

    TakeReader reader;
    CHECK(reader.open(temp.path, error));
    PacketKind kind{};
    StatePacket state;
    SchemaPacket schema;
    int read = 0;
    while (reader.next(kind, state, schema, error)) ++read;
    CHECK_EQ(read, 3);
    CHECK(error.find("middle of a record") != std::string::npos);
}

SVJ_TEST("take: a foreign file is refused") {
    TempFile temp("svj_take_foreign.scratchtake");
    {
        std::FILE* f = std::fopen(temp.path.c_str(), "wb");
        const char junk[64] = "definitely not a take";
        std::fwrite(junk, 1, sizeof(junk), f);
        std::fclose(f);
    }
    TakeReader reader;
    std::string error;
    CHECK(!reader.open(temp.path, error));
    CHECK(error.find("not a scratchvj take") != std::string::npos);
}

SVJ_TEST("take: a stub too short for a header is refused") {
    TempFile temp("svj_take_stub.scratchtake");
    {
        std::FILE* f = std::fopen(temp.path.c_str(), "wb");
        std::fwrite("SVT1", 1, 4, f);
        std::fclose(f);
    }
    TakeReader reader;
    std::string error;
    CHECK(!reader.open(temp.path, error));
    CHECK(error.find("too short") != std::string::npos);
}

SVJ_TEST("take: the schema travels with the take, so values keep their meaning") {
    // A take carrying values without their schema would be unreadable later.
    TempFile temp("svj_take_schema.scratchtake");
    Surface surface;
    surface.declare("xfader", ControlKind::Fader);
    surface.declare("ch1.filter", ControlKind::Knob);

    SchemaPacket schema;
    schema.entries = schema_from(surface);
    schema.schema_hash = schema_hash(schema.entries);

    StatePacket state = state_at(1, 1.0f);
    state.schema_hash = schema.schema_hash;

    std::string error;
    TakeWriter writer;
    CHECK(writer.open(temp.path, error));
    CHECK(writer.write_schema(schema, error));
    CHECK(writer.write_state(state, error));
    CHECK(writer.close(error));

    TakeReader reader;
    CHECK(reader.open(temp.path, error));
    PacketKind kind{};
    StatePacket read_state;
    SchemaPacket read_schema;
    CHECK(reader.next(kind, read_state, read_schema, error));
    CHECK(reader.next(kind, read_state, read_schema, error));
    CHECK_EQ(read_state.schema_hash, read_schema.schema_hash);
}
