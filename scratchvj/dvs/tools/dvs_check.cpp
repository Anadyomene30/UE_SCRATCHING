// scratchvj — which timecode definition, on which channel pair, actually locks.
//
// The roadmap refuses to guess between serato_2a, serato_2b and serato_cd, and
// this is how that promise is kept. It also settles the one thing the offline
// tests cannot: WHICH SIGN OF THE PHASE MEANS FORWARDS, which depends on how the
// RCA pair is wired and is a property of the desk, not of the code.
//
// It CAPTURES ONCE and then replays that buffer through every combination
// offline. Asking someone to spin a platter once per candidate would be nine
// definitions times twelve channel pairs of spinning; capturing once makes it
// one turn of the record for the whole matrix.
//
// Usage:
//   dvs_check <fragment du nom d'entree> [secondes]
//
// Turn the platter FORWARDS for the whole capture. The report names the pair,
// the definition, and whether the pitch came out positive -- if it is negative
// while the record went forwards, the channels are swapped and that is the
// calibration answer.
#define NOMINMAX
#include <windows.h>

#include <initguid.h>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "dvs/decoder.h"

using namespace svj;

namespace {

std::string narrow(const wchar_t* text) {
    if (text == nullptr) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size > 0 ? size - 1 : 0), '\0');
    if (size > 1) {
        WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), size, nullptr, nullptr);
    }
    return out;
}

std::string endpoint_name(IMMDevice* device) {
    IPropertyStore* store = nullptr;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store))) return {};
    PROPVARIANT value;
    PropVariantInit(&value);
    std::string name;
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) &&
        value.vt == VT_LPWSTR) {
        name = narrow(value.pwszVal);
    }
    PropVariantClear(&value);
    store->Release();
    return name;
}

}  // namespace

int main(int argc, char** argv) {
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return 1;

    IMMDeviceEnumerator* enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator)))) {
        std::fprintf(stderr, "enumerateur audio indisponible\n");
        return 1;
    }
    IMMDeviceCollection* collection = nullptr;
    enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    UINT count = 0;
    if (collection != nullptr) collection->GetCount(&count);

    if (argc < 2) {
        std::printf("%u entrees :\n", count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* device = nullptr;
            collection->Item(i, &device);
            std::printf("  %u : %s\n", i, endpoint_name(device).c_str());
            device->Release();
        }
        std::printf("\nusage : dvs_check <nom> [secondes]\n");
        return 0;
    }

    IMMDevice* device = nullptr;
    std::string chosen;
    const std::string wanted = argv[1];
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* candidate = nullptr;
        collection->Item(i, &candidate);
        std::string lower_name = endpoint_name(candidate), lower_want = wanted;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        std::transform(lower_want.begin(), lower_want.end(), lower_want.begin(), ::tolower);
        if (lower_name.find(lower_want) != std::string::npos) {
            device = candidate;
            chosen = endpoint_name(candidate);
            break;
        }
        candidate->Release();
    }
    if (device == nullptr) {
        std::fprintf(stderr, "aucune entree ne correspond a \"%s\"\n", argv[1]);
        return 1;
    }
    const double seconds = argc > 2 ? std::stod(argv[2]) : 6.0;

    IAudioClient* client = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&client)))) {
        std::fprintf(stderr, "activation impossible\n");
        return 1;
    }
    WAVEFORMATEX* format = nullptr;
    if (FAILED(client->GetMixFormat(&format)) || format == nullptr) {
        std::fprintf(stderr, "pas de format mixe\n");
        return 1;
    }
    // SHARED, never exclusive: Serato may be on this interface.
    const HRESULT opened =
        client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 1000000, 0, format, nullptr);
    if (FAILED(opened)) {
        std::fprintf(stderr, "ouverture refusee (0x%08lX)\n",
                     static_cast<unsigned long>(opened));
        return 1;
    }

    IAudioCaptureClient* capture = nullptr;
    client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture));

    const auto rate = static_cast<unsigned>(format->nSamplesPerSec);
    const unsigned channels = format->nChannels;
    const bool is_float = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                          (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                           reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat ==
                               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    std::printf("\"%s\" : %u Hz, %u canaux\n", chosen.c_str(), rate, channels);
    std::printf("TOURNE LE PLATEAU EN AVANT pendant %.0f s...\n\n", seconds);
    std::fflush(stdout);

    // Captured once, de-interleaved, then replayed offline through the whole
    // matrix below.
    std::vector<std::vector<float>> lanes(channels);
    client->Start();
    const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0);
    while (GetTickCount64() < deadline) {
        UINT32 available = 0;
        capture->GetNextPacketSize(&available);
        while (available > 0) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
            for (UINT32 f = 0; f < frames; ++f) {
                for (unsigned c = 0; c < channels; ++c) {
                    float value = 0.0f;
                    if (!silent) {
                        if (is_float) {
                            value = reinterpret_cast<const float*>(data)[f * channels + c];
                        } else if (format->wBitsPerSample == 16) {
                            value = reinterpret_cast<const std::int16_t*>(
                                        data)[f * channels + c] / 32768.0f;
                        }
                    }
                    lanes[c].push_back(value);
                }
            }
            capture->ReleaseBuffer(frames);
            capture->GetNextPacketSize(&available);
        }
        Sleep(5);
    }
    client->Stop();

    if (lanes.empty() || lanes[0].empty()) {
        std::fprintf(stderr, "aucun echantillon capture\n");
        return 2;
    }
    const std::size_t frames = lanes[0].size();

    // Saved before anything is analysed, so a capture that fails to lock can be
    // worked on again and again without asking anyone to spin a record each
    // time. This is the difference between debugging and guessing.
    {
        std::vector<std::int16_t> interleaved(frames * channels);
        for (std::size_t i = 0; i < frames; ++i) {
            for (unsigned c = 0; c < channels; ++c) {
                const float v = std::clamp(lanes[c][i], -1.0f, 1.0f);
                interleaved[i * channels + c] = static_cast<std::int16_t>(v * 32767.0f);
            }
        }
        FILE* file = std::fopen("dvs_capture.wav", "wb");
        if (file != nullptr) {
            const std::uint32_t data_bytes =
                static_cast<std::uint32_t>(interleaved.size() * 2);
            const std::uint32_t byte_rate = rate * channels * 2;
            const std::uint16_t block_align = static_cast<std::uint16_t>(channels * 2);
            const std::uint32_t riff = 36 + data_bytes;
            const std::uint32_t fmt_size = 16;
            const std::uint16_t pcm = 1, bits = 16;
            const auto ch16 = static_cast<std::uint16_t>(channels);
            std::fwrite("RIFF", 1, 4, file);
            std::fwrite(&riff, 4, 1, file);
            std::fwrite("WAVEfmt ", 1, 8, file);
            std::fwrite(&fmt_size, 4, 1, file);
            std::fwrite(&pcm, 2, 1, file);
            std::fwrite(&ch16, 2, 1, file);
            std::fwrite(&rate, 4, 1, file);
            std::fwrite(&byte_rate, 4, 1, file);
            std::fwrite(&block_align, 2, 1, file);
            std::fwrite(&bits, 2, 1, file);
            std::fwrite("data", 1, 4, file);
            std::fwrite(&data_bytes, 4, 1, file);
            std::fwrite(interleaved.data(), 2, interleaved.size(), file);
            std::fclose(file);
            std::printf("capture enregistree : dvs_capture.wav (%u canaux, %.1f s)\n\n",
                        channels, static_cast<double>(frames) / rate);
        }
    }

    // --- the matrix ----------------------------------------------------------
    struct Hit {
        unsigned pair = 0;
        bool swapped = false;
        bool phono = false;
        std::string definition;
        double position_s = 0.0;
        float pitch = 0.0f;
        float level = 0.0f;
    };
    std::vector<Hit> hits;
    std::vector<float> stereo;

    for (unsigned c = 0; c + 1 < channels; c += 2) {
        // Skip a silent pair outright: nine decoders over a channel of zeros is
        // just time.
        float peak = 0.0f;
        for (std::size_t i = 0; i < frames; ++i) {
            peak = std::max(peak, std::max(std::fabs(lanes[c][i]), std::fabs(lanes[c + 1][i])));
        }
        if (peak < 0.001f) continue;
        std::printf("  voies %u/%u : crete %.3f\n", c + 1, c + 2, peak);

        // Both channel orders and both threshold settings. Order decides which
        // channel xwax treats as primary -- which is the one it reads bits off,
        // so the wrong way round does not merely invert direction, it can stop
        // the bitstream resolving at all. And `phono` is a threshold: too high
        // for a quiet signal and nothing crosses.
        for (int swapped = 0; swapped < 2; ++swapped) {
            stereo.resize(frames * 2);
            for (std::size_t i = 0; i < frames; ++i) {
                stereo[i * 2] = swapped ? lanes[c + 1][i] : lanes[c][i];
                stereo[i * 2 + 1] = swapped ? lanes[c][i] : lanes[c + 1][i];
            }

            for (int phono = 0; phono < 2; ++phono) {
                for (const std::string& name : svj::dvs::known_definitions()) {
                    svj::dvs::TimecodeDecoder decoder;
                    if (!decoder.open(name, rate, phono != 0)) continue;

                    DecoderSample last;
                    const std::size_t block = 512;
                    for (std::size_t i = 0; i < frames; i += block) {
                        const std::size_t n = std::min(block, frames - i);
                        last = decoder.submit(stereo.data() + i * 2, n,
                                              static_cast<double>(i) / rate);
                    }
                    if (last.position_s >= 0.0) {
                        hits.push_back({c, swapped != 0, phono != 0, name,
                                        last.position_s, last.pitch,
                                        last.signal_level});
                    }
                }
            }
        }
    }
    std::printf("\n");

    if (hits.empty()) {
        std::printf("=> AUCUN VERROUILLAGE.\n");
        std::printf("   Ni une paire ni une definition n'a accroche. Verifie que la\n");
        std::printf("   remote tournait, que le recepteur est appaire, et que ses RCA\n");
        std::printf("   arrivent bien sur cette entree.\n");
        return 3;
    }

    std::printf("%-8s %-8s %-7s %-16s %12s %8s %8s\n", "voies", "ordre", "seuil",
                "definition", "position", "pitch", "niveau");
    for (const Hit& hit : hits) {
        char pair[16];
        std::snprintf(pair, sizeof(pair), "%u/%u", hit.pair + 1, hit.pair + 2);
        std::printf("%-8s %-8s %-7s %-16s %10.2f s %8.3f %8.3f\n", pair,
                    hit.swapped ? "inverse" : "direct", hit.phono ? "phono" : "ligne",
                    hit.definition.c_str(), hit.position_s, hit.pitch, hit.level);
    }

    const Hit& best = hits.front();
    std::printf("\n=> VERROUILLE : %s sur les voies %u/%u (%s, seuil %s).\n",
                best.definition.c_str(), best.pair + 1, best.pair + 2,
                best.swapped ? "canaux inverses" : "canaux directs",
                best.phono ? "phono" : "ligne");
    if (best.pitch < 0.0f) {
        std::printf("   Le pitch est NEGATIF alors que le plateau tournait en avant :\n");
        std::printf("   les deux voies sont inversees. Il faut soit echanger les RCA,\n");
        std::printf("   soit echanger les canaux a la lecture.\n");
    } else {
        std::printf("   Pitch positif en avant : le cablage est dans le bon sens,\n");
        std::printf("   aucune inversion a compenser.\n");
    }
    return 0;
}
