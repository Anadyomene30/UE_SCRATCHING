// scratchvj — the quadrature tracker against the real carrier.
//
// core/quadrature is tested against a generated signal, which proves the maths.
// This proves the WIRING, and it settles the two facts the code must never
// guess: which channel pair carries the carrier, and which sign of the velocity
// means forwards. Both are properties of the desk, not of the algorithm.
//
// It captures once and replays offline through every channel pair, so one turn
// of the platter answers the whole question rather than one guess per spin.
//
// Turn the platter FORWARDS for the whole capture.
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

#include "core/quadrature.h"

using namespace svj;

namespace {

constexpr double kPi = 3.14159265358979323846;

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

// The carrier itself has to be measured, not assumed: it is a scale calibration,
// and getting it wrong by a factor scales every velocity by that factor. Serato
// is 1000 Hz, Traktor 2000, MixVibes 1300.
double dominant_hz(const std::vector<float>& x, double rate) {
    if (x.size() < 4) return 0.0;
    double best_hz = 0.0, best = 0.0;
    for (double hz = 500.0; hz <= 2600.0; hz += 5.0) {
        double re = 0.0, im = 0.0;
        const std::size_t n = std::min<std::size_t>(x.size(), 20000);
        for (std::size_t i = 0; i < n; ++i) {
            const double a = 2.0 * kPi * hz * static_cast<double>(i) / rate;
            re += x[i] * std::cos(a);
            im += x[i] * std::sin(a);
        }
        const double magnitude = std::sqrt(re * re + im * im);
        if (magnitude > best) {
            best = magnitude;
            best_hz = hz;
        }
    }
    return best_hz;
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
        std::printf("\nusage : quad_check <nom> [secondes]\n");
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

    const double rate = format->nSamplesPerSec;
    const unsigned channels = format->nChannels;
    const bool is_float = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                          (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                           reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat ==
                               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    std::printf("\"%s\" : %.0f Hz, %u canaux\n", chosen.c_str(), rate, channels);
    std::printf("TOURNE LE PLATEAU EN AVANT pendant %.0f s...\n\n", seconds);
    std::fflush(stdout);

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
    const double captured_s = static_cast<double>(frames) / rate;

    struct Hit {
        unsigned pair = 0;
        double carrier_hz = 0.0;
        double position_s = 0.0;
        float velocity = 0.0f;
        float level = 0.0f;
        std::uint32_t slews = 0;
    };
    std::vector<Hit> hits;
    std::vector<float> stereo;

    for (unsigned c = 0; c + 1 < channels; c += 2) {
        float peak = 0.0f;
        for (std::size_t i = 0; i < frames; ++i) {
            peak = std::max(peak, std::max(std::fabs(lanes[c][i]), std::fabs(lanes[c + 1][i])));
        }
        if (peak < 0.005f) continue;

        const double carrier = dominant_hz(lanes[c], rate);
        if (carrier <= 0.0) continue;

        stereo.resize(frames * 2);
        for (std::size_t i = 0; i < frames; ++i) {
            stereo[i * 2] = lanes[c][i];
            stereo[i * 2 + 1] = lanes[c + 1][i];
        }

        QuadratureConfig config;
        config.carrier_hz = carrier;
        config.sample_rate = rate;
        QuadratureTracker tracker;
        if (!tracker.configure(config)) continue;

        const std::size_t block = 512;
        for (std::size_t i = 0; i < frames; i += block) {
            const std::size_t n = std::min(block, frames - i);
            tracker.submit(stereo.data() + i * 2, n, static_cast<double>(i) / rate);
        }
        hits.push_back({c, carrier, tracker.position_s(), tracker.velocity(),
                        tracker.level(), tracker.slew_events()});
    }

    if (hits.empty()) {
        std::printf("=> AUCUNE PORTEUSE.\n");
        std::printf("   Verifie que la remote tournait et que les RCA du recepteur\n");
        std::printf("   arrivent bien sur cette entree.\n");
        return 3;
    }

    std::printf("%-8s %10s %12s %9s %8s %7s\n", "voies", "porteuse", "parcouru",
                "vitesse", "niveau", "slew");
    for (const Hit& hit : hits) {
        char pair[16];
        std::snprintf(pair, sizeof(pair), "%u/%u", hit.pair + 1, hit.pair + 2);
        std::printf("%-8s %8.0f Hz %10.3f s %9.3f %8.3f %7u\n", pair, hit.carrier_hz,
                    hit.position_s, hit.velocity, hit.level, hit.slews);
    }

    const Hit& best = hits.front();
    std::printf("\n=> PORTEUSE SUIVIE sur les voies %u/%u, %.0f Hz.\n", best.pair + 1,
                best.pair + 2, best.carrier_hz);
    std::printf("   %.2f s de disque parcourus en %.2f s de capture.\n", best.position_s,
                captured_s);
    if (best.velocity < 0.0f) {
        std::printf("   VITESSE NEGATIVE alors que le plateau tournait en avant :\n");
        std::printf("   les deux voies sont inversees. Echanger les RCA, ou echanger\n");
        std::printf("   les canaux a la lecture.\n");
    } else {
        std::printf("   Vitesse positive en avant : cablage dans le bon sens.\n");
    }
    if (best.slews > 0) {
        std::printf("   %u depassements de vitesse : le plateau est alle plus vite que\n",
                    best.slews);
        std::printf("   ce que %.0f Hz peut suivre. Position perdue d'autant.\n", rate);
    }
    return 0;
}
