#include "core/midi.h"

namespace svj {
namespace {

constexpr std::uint8_t kStatusMask = 0xF0;
constexpr std::uint8_t kChannelMask = 0x0F;

// Number of data bytes each channel-voice status expects.
std::uint8_t data_bytes_for(std::uint8_t status) {
    switch (status & kStatusMask) {
        case 0x80:  // note off
        case 0x90:  // note on
        case 0xA0:  // polyphonic aftertouch
        case 0xB0:  // control change
        case 0xE0:  // pitch bend
            return 2;
        case 0xC0:  // program change
        case 0xD0:  // channel aftertouch
            return 1;
        default:
            return 0;
    }
}

bool is_realtime(std::uint8_t byte) { return byte >= 0xF8; }
bool is_status(std::uint8_t byte) { return (byte & 0x80) != 0; }

}  // namespace

float MidiEvent::normalised() const {
    const float span = high_resolution ? 16383.0f : 127.0f;
    return static_cast<float>(value) / span;
}

void MidiDecoder::reset() {
    status_ = 0;
    data_count_ = 0;
    expected_data_ = 0;
    have_pending_msb_ = false;
}

void MidiDecoder::feed(const std::uint8_t* bytes, std::size_t count, std::vector<MidiEvent>& out) {
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t byte = bytes[i];

        // Realtime messages may interleave anywhere and must not disturb the
        // running status or a partially received message.
        if (is_realtime(byte)) continue;

        if (is_status(byte)) {
            if (byte >= 0xF0) {
                // System common cancels running status.
                status_ = 0;
                expected_data_ = 0;
                data_count_ = 0;
                have_pending_msb_ = false;
                continue;
            }
            status_ = byte;
            expected_data_ = data_bytes_for(byte);
            data_count_ = 0;
            continue;
        }

        if (status_ == 0) continue;  // data byte with no status yet

        data_[data_count_++] = byte;
        if (data_count_ >= expected_data_) {
            dispatch(out);
            data_count_ = 0;  // running status: keep status_ for the next message
        }
    }
}

void MidiDecoder::dispatch(std::vector<MidiEvent>& out) {
    const std::uint8_t type = status_ & kStatusMask;
    const std::uint8_t channel = status_ & kChannelMask;

    MidiEvent ev;
    ev.channel = channel;

    switch (type) {
        case 0x80:
            ev.kind = MidiKind::NoteOff;
            ev.number = data_[0];
            ev.value = data_[1];
            have_pending_msb_ = false;
            out.push_back(ev);
            return;

        case 0x90:
            // Note on with zero velocity is the conventional note off.
            ev.kind = data_[1] == 0 ? MidiKind::NoteOff : MidiKind::NoteOn;
            ev.number = data_[0];
            ev.value = data_[1];
            have_pending_msb_ = false;
            out.push_back(ev);
            return;

        case 0xE0:
            ev.kind = MidiKind::PitchBend;
            ev.value = static_cast<std::uint16_t>(data_[0] | (data_[1] << 7));
            ev.high_resolution = true;
            have_pending_msb_ = false;
            out.push_back(ev);
            return;

        case 0xB0: {
            const std::uint8_t number = data_[0];
            const std::uint8_t value = data_[1];

            // An LSB directly following its MSB upgrades that control to 14 bits.
            if (number >= 32 && number < 64 && have_pending_msb_ &&
                pending_channel_ == channel && pending_number_ == number - 32) {
                MidiEvent hi;
                hi.kind = MidiKind::ControlChange;
                hi.channel = channel;
                hi.number = pending_number_;
                hi.value = static_cast<std::uint16_t>((pending_msb_ << 7) | value);
                hi.high_resolution = true;
                have_pending_msb_ = false;
                out.push_back(hi);
                return;
            }

            ev.kind = MidiKind::ControlChange;
            ev.number = number;
            ev.value = value;
            out.push_back(ev);

            if (number < 32) {
                have_pending_msb_ = true;
                pending_channel_ = channel;
                pending_number_ = number;
                pending_msb_ = value;
            } else {
                have_pending_msb_ = false;
            }
            return;
        }

        default:
            ev.kind = MidiKind::Other;
            ev.number = data_[0];
            ev.value = expected_data_ > 1 ? data_[1] : 0;
            have_pending_msb_ = false;
            out.push_back(ev);
            return;
    }
}

}  // namespace svj
