#include "core/midi.h"
#include "harness.h"

using namespace svj;

namespace {

std::vector<MidiEvent> decode(std::initializer_list<std::uint8_t> bytes) {
    MidiDecoder decoder;
    std::vector<MidiEvent> out;
    const std::vector<std::uint8_t> buffer(bytes);
    decoder.feed(buffer.data(), buffer.size(), out);
    return out;
}

}  // namespace

SVJ_TEST("midi: control change carries channel, number and value") {
    const auto events = decode({0xB3, 0x07, 0x40});
    CHECK_EQ(events.size(), std::size_t{1});
    CHECK(events[0].kind == MidiKind::ControlChange);
    CHECK_EQ(int(events[0].channel), 3);
    CHECK_EQ(int(events[0].number), 7);
    CHECK_EQ(int(events[0].value), 64);
    CHECK(!events[0].high_resolution);
}

SVJ_TEST("midi: note on with zero velocity is a note off") {
    const auto events = decode({0x90, 0x3C, 0x00});
    CHECK_EQ(events.size(), std::size_t{1});
    CHECK(events[0].kind == MidiKind::NoteOff);
}

SVJ_TEST("midi: running status reuses the previous status byte") {
    const auto events = decode({0xB0, 0x10, 0x01, 0x10, 0x02, 0x10, 0x03});
    CHECK_EQ(events.size(), std::size_t{3});
    for (const MidiEvent& e : events) CHECK(e.kind == MidiKind::ControlChange);
    CHECK_EQ(int(events[2].value), 3);
}

SVJ_TEST("midi: an MSB alone still produces a 7-bit event") {
    // Controllers that never send an LSB must not be silently swallowed.
    const auto events = decode({0xB0, 0x0A, 0x7F});
    CHECK_EQ(events.size(), std::size_t{1});
    CHECK(!events[0].high_resolution);
    CHECK_EQ(int(events[0].value), 127);
}

SVJ_TEST("midi: an LSB following its MSB upgrades the pair to 14 bits") {
    // CC 10 then CC 42 (10 + 32) on the same channel.
    const auto events = decode({0xB0, 0x0A, 0x01, 0xB0, 0x2A, 0x02});
    CHECK_EQ(events.size(), std::size_t{2});
    CHECK(!events[0].high_resolution);
    CHECK(events[1].high_resolution);
    CHECK_EQ(int(events[1].number), 10);
    CHECK_EQ(int(events[1].value), (1 << 7) | 2);
    CHECK_NEAR(events[1].normalised(), 130.0 / 16383.0, 1e-6);
}

SVJ_TEST("midi: an LSB on a different channel does not pair") {
    const auto events = decode({0xB0, 0x0A, 0x01, 0xB1, 0x2A, 0x02});
    CHECK_EQ(events.size(), std::size_t{2});
    CHECK(!events[1].high_resolution);
}

SVJ_TEST("midi: realtime bytes interleave without corrupting a message") {
    // A clock byte lands between the status and its data bytes.
    const auto events = decode({0xB0, 0xF8, 0x0C, 0xF8, 0x55});
    CHECK_EQ(events.size(), std::size_t{1});
    CHECK_EQ(int(events[0].number), 12);
    CHECK_EQ(int(events[0].value), 85);
}

SVJ_TEST("midi: system common cancels running status") {
    const auto events = decode({0xB0, 0x10, 0x01, 0xF1, 0x00, 0x10, 0x02});
    // Only the first CC survives; the trailing data bytes have no status.
    CHECK_EQ(events.size(), std::size_t{1});
}

SVJ_TEST("midi: pitch bend is assembled from both data bytes") {
    const auto events = decode({0xE0, 0x00, 0x40});
    CHECK_EQ(events.size(), std::size_t{1});
    CHECK(events[0].kind == MidiKind::PitchBend);
    CHECK_EQ(int(events[0].value), 8192);
    CHECK(events[0].high_resolution);
}

SVJ_TEST("midi: a message split across two feeds is reassembled") {
    MidiDecoder decoder;
    std::vector<MidiEvent> out;
    const std::uint8_t first[] = {0xB0, 0x0C};
    const std::uint8_t second[] = {0x7F};
    decoder.feed(first, 2, out);
    CHECK(out.empty());
    decoder.feed(second, 1, out);
    CHECK_EQ(out.size(), std::size_t{1});
    CHECK_EQ(int(out[0].value), 127);
}
