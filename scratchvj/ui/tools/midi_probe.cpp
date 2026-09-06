// scratchvj — what a real controller actually sends, and when.
//
// This exists to settle the first of the two questions the roadmap leaves to the
// hardware: DOES THE MIXER EMIT ITS STATE WHEN YOU CONNECT? The answer decides
// whether the ghost-control machinery in core/surface is a permanent feature of
// the instrument or a workaround for a gap that closes itself. A guess either
// way costs a redesign later, so it is measured.
//
// It also puts core/midi in front of real bytes for the first time. The decoder
// has been unit-tested since the first commit -- running status, realtime
// interleave, 14-bit pairs -- but a controller that violates an assumption would
// show up here and nowhere else.
//
// Usage:
//   midi_probe                 list the inputs
//   midi_probe ELITE [sec]     listen to one, default 12 seconds
//
// The report separates the FIRST HALF SECOND from the rest, because that is the
// window a connection-time state dump would land in. Anything arriving later is
// a hand moving something.
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "core/midi.h"

namespace {

struct Message {
    double at_s = 0.0;
    UINT device = 0;
    std::uint8_t bytes[3] = {0, 0, 0};
};

std::mutex g_lock;
std::vector<Message> g_messages;
std::chrono::steady_clock::time_point g_opened;

void CALLBACK on_midi(HMIDIIN, UINT status, DWORD_PTR user, DWORD_PTR param1, DWORD_PTR) {
    if (status != MIM_DATA) return;
    const auto now = std::chrono::steady_clock::now();
    Message message;
    message.device = static_cast<UINT>(user);
    message.at_s = std::chrono::duration<double>(now - g_opened).count();
    const auto packed = static_cast<std::uint32_t>(param1);
    message.bytes[0] = static_cast<std::uint8_t>(packed & 0xFF);
    message.bytes[1] = static_cast<std::uint8_t>((packed >> 8) & 0xFF);
    message.bytes[2] = static_cast<std::uint8_t>((packed >> 16) & 0xFF);
    std::lock_guard<std::mutex> guard(g_lock);
    g_messages.push_back(message);
}

std::string name_of(UINT index) {
    MIDIINCAPSA caps{};
    if (midiInGetDevCapsA(index, &caps, sizeof(caps)) != MMSYSERR_NOERROR) return {};
    return caps.szPname;
}

const char* kind_name(svj::MidiKind kind) {
    switch (kind) {
        case svj::MidiKind::NoteOn: return "note on";
        case svj::MidiKind::NoteOff: return "note off";
        case svj::MidiKind::ControlChange: return "cc";
        case svj::MidiKind::PitchBend: return "pitch";
        default: return "autre";
    }
}

// How many data bytes a status byte introduces. winmm hands over whole short
// messages, so this is only used to trim the padding byte off a two-byte one --
// feeding that zero to the decoder would invent a value the device never sent.
std::size_t data_bytes(std::uint8_t status) {
    const std::uint8_t high = status & 0xF0;
    if (high == 0xC0 || high == 0xD0) return 1;
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    const UINT count = midiInGetNumDevs();
    if (argc < 2) {
        std::printf("%u entrees MIDI :\n", count);
        for (UINT i = 0; i < count; ++i) {
            std::printf("  %u : %s\n", i, name_of(i).c_str());
        }
        std::printf("\nusage : midi_probe <nom ou index> [secondes]\n");
        return 0;
    }

    // "all" opens every input at once. That is the useful mode with real gear on
    // the desk: the performer sweeps everything ONCE and every device is
    // characterised, instead of being asked to do it again per port.
    std::vector<UINT> chosen;
    const std::string wanted = argv[1];
    if (wanted == "all" || wanted == "tout") {
        for (UINT i = 0; i < count; ++i) chosen.push_back(i);
    } else if (!wanted.empty() &&
               wanted.find_first_not_of("0123456789") == std::string::npos) {
        chosen.push_back(static_cast<UINT>(std::stoul(wanted)));
    } else {
        // By name fragment, so "ELITE" finds "ELITE" and "RP8" finds
        // "RP8000mk2" without anyone typing it exactly.
        for (UINT i = 0; i < count; ++i) {
            std::string lower_name = name_of(i), lower_want = wanted;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            std::transform(lower_want.begin(), lower_want.end(), lower_want.begin(), ::tolower);
            if (lower_name.find(lower_want) != std::string::npos) chosen.push_back(i);
        }
    }
    if (chosen.empty() || chosen.front() >= count) {
        std::fprintf(stderr, "aucune entree MIDI ne correspond a \"%s\"\n", argv[1]);
        return 1;
    }
    const double seconds = argc > 2 ? std::stod(argv[2]) : 12.0;

    std::printf("ecoute pendant %.0f s :\n", seconds);
    for (UINT index : chosen) std::printf("  %u : %s\n", index, name_of(index).c_str());
    std::printf("\nne touche a rien pendant les 3 premieres secondes, puis bouge tout.\n\n");
    std::fflush(stdout);

    // The clock starts BEFORE any port opens, so a device that dumps its state
    // the instant it is opened is timed from the right zero.
    g_opened = std::chrono::steady_clock::now();
    std::vector<HMIDIIN> handles;
    std::vector<UINT> live;
    for (UINT index : chosen) {
        HMIDIIN handle = nullptr;
        // The device index travels as the callback's user word, so messages can
        // be attributed when several ports are open at once.
        if (midiInOpen(&handle, index, reinterpret_cast<DWORD_PTR>(on_midi),
                       static_cast<DWORD_PTR>(index),
                       CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            // A port another application already holds is a FINDING, not a
            // failure: it is exactly what Serato holding the mixer looks like.
            std::printf("  (%u %s : deja pris par une autre application)\n", index,
                        name_of(index).c_str());
            continue;
        }
        midiInStart(handle);
        handles.push_back(handle);
        live.push_back(index);
    }
    if (handles.empty()) {
        std::fprintf(stderr, "aucun port n'a pu etre ouvert\n");
        return 1;
    }

    // Reported as it happens, not only at the end. A probe that stays silent for
    // half a minute is indistinguishable from a broken one, and the person
    // sweeping the controls needs to see that their hand is landing.
    const auto until = g_opened + std::chrono::duration<double>(seconds);
    std::size_t reported = 0;
    while (std::chrono::steady_clock::now() < until) {
        Sleep(50);
        std::vector<Message> fresh;
        {
            std::lock_guard<std::mutex> guard(g_lock);
            if (g_messages.size() > reported) {
                fresh.assign(g_messages.begin() + static_cast<std::ptrdiff_t>(reported),
                             g_messages.end());
                reported = g_messages.size();
            }
        }
        for (const Message& message : fresh) {
            std::printf("  %6.2fs  %-18s %02X %02X %02X\n", message.at_s,
                        name_of(message.device).c_str(), message.bytes[0],
                        message.bytes[1], message.bytes[2]);
        }
        if (!fresh.empty()) std::fflush(stdout);
    }

    for (HMIDIIN handle : handles) {
        midiInStop(handle);
        midiInClose(handle);
    }

    std::vector<Message> messages;
    {
        std::lock_guard<std::mutex> guard(g_lock);
        messages = g_messages;
    }

    if (messages.empty()) {
        std::printf("aucun message recu.\n");
        return 2;
    }

    struct Seen {
        int count = 0;
        double first_s = 0.0;
        std::uint16_t last = 0;
    };
    struct Device {
        // One decoder PER PORT: running status is a property of a single byte
        // stream, and sharing one decoder across ports would let a status byte
        // from one device interpret the data bytes of another.
        svj::MidiDecoder decoder;
        std::map<std::string, Seen> controls;
        int total = 0;
        int in_first_half = 0;
        int undecoded = 0;
    };
    std::map<UINT, Device> devices;
    for (UINT index : live) devices[index];

    for (const Message& message : messages) {
        Device& device = devices[message.device];
        std::vector<svj::MidiEvent> events;
        const std::size_t length = 1 + data_bytes(message.bytes[0]);
        device.decoder.feed(message.bytes, length, events);
        ++device.total;
        if (events.empty()) ++device.undecoded;
        if (message.at_s < 0.5) ++device.in_first_half;

        for (const svj::MidiEvent& event : events) {
            char key[64];
            std::snprintf(key, sizeof(key), "%-9s ch%-2u n%-3u", kind_name(event.kind),
                          static_cast<unsigned>(event.channel) + 1,
                          static_cast<unsigned>(event.number));
            Seen& seen = device.controls[key];
            if (seen.count == 0) seen.first_s = message.at_s;
            ++seen.count;
            seen.last = event.value;
        }
    }

    for (const auto& entry : devices) {
        const Device& device = entry.second;
        std::printf("=== %s ===\n", name_of(entry.first).c_str());
        if (device.total == 0) {
            std::printf("  muet pendant tout le test.\n\n");
            continue;
        }
        std::printf("  %d messages, %zu controles distincts, %d dans la 1re demi-seconde\n",
                    device.total, device.controls.size(), device.in_first_half);

        // THE ANSWER, stated rather than left to be inferred from the numbers.
        // Eight is the threshold because a state dump is a SWEEP of the surface;
        // one or two messages at startup is a heartbeat or a handshake, not a
        // description of where the knobs are.
        if (device.in_first_half >= 8) {
            std::printf("  => EMET SON ETAT A L'OUVERTURE DU PORT.\n");
            std::printf("     Les potards absolus n'ont pas besoin du mode fantome.\n");
        } else {
            std::printf("  => PAS DE DUMP D'ETAT (%d message(s) au demarrage).\n",
                        device.in_first_half);
            std::printf("     Le mode fantome de core/surface reste necessaire.\n");
        }

        std::printf("  %-28s %8s %10s %8s\n", "controle", "messages", "1er (s)",
                    "derniere");
        for (const auto& control : device.controls) {
            std::printf("  %-28s %8d %10.2f %8u\n", control.first.c_str(),
                        control.second.count, control.second.first_s,
                        static_cast<unsigned>(control.second.last));
        }
        if (device.undecoded > 0) {
            std::printf("  %d message(s) non traduits par core/midi (sysex, realtime).\n",
                        device.undecoded);
        }
        std::printf("\n");
    }
    return 0;
}
