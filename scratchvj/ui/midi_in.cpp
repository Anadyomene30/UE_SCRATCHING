#include "midi_in.h"

#if defined(_WIN32)

#include <windows.h>

#include <algorithm>
#include <cctype>

namespace svj::ui {
namespace {

std::string lowered(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

// How many data bytes a status byte introduces. winmm hands over whole short
// messages padded to three, so this trims the padding off a two-byte one --
// feeding that zero to the decoder would invent a value the device never sent.
std::size_t data_bytes(std::uint8_t status) {
    const std::uint8_t high = status & 0xF0;
    return (high == 0xC0 || high == 0xD0) ? 1 : 2;
}

void CALLBACK on_midi(HMIDIIN, UINT status, DWORD_PTR user, DWORD_PTR param1, DWORD_PTR) {
    auto* self = reinterpret_cast<MidiInput*>(user);
    if (self == nullptr) return;
    if (status == MIM_DATA) {
        self->push_raw(static_cast<std::uint32_t>(param1));
    } else if (status == MIM_CLOSE) {
        // The driver closed the handle under us: the cable was pulled. Not
        // every driver sends this, which is why the rig also re-enumerates,
        // but when it arrives it is the fastest word we get.
        self->mark_closed();
    }
}

}  // namespace

std::vector<std::string> MidiInput::ports() {
    std::vector<std::string> names;
    const UINT count = midiInGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        MIDIINCAPSA caps{};
        if (midiInGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            names.emplace_back(caps.szPname);
        }
    }
    return names;
}

MidiInput::~MidiInput() { close(); }

bool MidiInput::open(const std::string& port, int ordinal, std::uint8_t device) {
    close();

    const UINT count = midiInGetNumDevs();
    const std::string want = lowered(port);
    if (want.empty()) return false;
    UINT chosen = count;
    int seen = 0;
    for (UINT i = 0; i < count; ++i) {
        MIDIINCAPSA caps{};
        if (midiInGetDevCapsA(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR) continue;
        if (lowered(caps.szPname).find(want) == std::string::npos) continue;
        if (seen == ordinal) {
            chosen = i;
            port_ = caps.szPname;
            break;
        }
        ++seen;
    }
    if (chosen >= count) return false;

    HMIDIIN handle = nullptr;
    if (midiInOpen(&handle, chosen, reinterpret_cast<DWORD_PTR>(on_midi),
                   reinterpret_cast<DWORD_PTR>(this),
                   CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        port_.clear();
        return false;
    }
    handle_ = handle;
    device_ = device;
    closed_.store(false);
    midiInStart(handle);
    return true;
}

void MidiInput::close() {
    if (handle_ != nullptr) {
        auto handle = static_cast<HMIDIIN>(handle_);
        midiInStop(handle);
        midiInClose(handle);
        handle_ = nullptr;
    }
    port_.clear();
    closed_.store(false);
    std::lock_guard<std::mutex> guard(lock_);
    bytes_.clear();
    messages_ = 0;
    decoder_.reset();
}

void MidiInput::push_raw(std::uint32_t packed) {
    const std::uint8_t status = static_cast<std::uint8_t>(packed & 0xFF);
    const std::size_t length = 1 + data_bytes(status);
    std::lock_guard<std::mutex> guard(lock_);
    bytes_.push_back(status);
    if (length > 1) bytes_.push_back(static_cast<std::uint8_t>((packed >> 8) & 0xFF));
    if (length > 2) bytes_.push_back(static_cast<std::uint8_t>((packed >> 16) & 0xFF));
    ++messages_;
}

void MidiInput::drain(std::vector<MidiEvent>& out) {
    out.clear();
    std::vector<std::uint8_t> raw;
    {
        std::lock_guard<std::mutex> guard(lock_);
        raw.swap(bytes_);
    }
    if (!raw.empty()) decoder_.feed(raw.data(), raw.size(), out);
    for (MidiEvent& event : out) event.device = device_;
}

std::uint64_t MidiInput::message_count() const {
    std::lock_guard<std::mutex> guard(lock_);
    return messages_;
}

}  // namespace svj::ui

#else

namespace svj::ui {
MidiInput::~MidiInput() = default;
std::vector<std::string> MidiInput::ports() { return {}; }
bool MidiInput::open(const std::string&, int, std::uint8_t) { return false; }
void MidiInput::close() {}
void MidiInput::push_raw(std::uint32_t) {}
void MidiInput::drain(std::vector<MidiEvent>& out) { out.clear(); }
std::uint64_t MidiInput::message_count() const { return 0; }
}  // namespace svj::ui

#endif
