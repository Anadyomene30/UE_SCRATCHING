// scratchvj — what actually arrives on an audio input, and whether it is timecode.
//
// This settles the second of the two questions the roadmap leaves to the
// hardware, and it settles the one the whole audio architecture rests on: CAN
// THIS APPLICATION READ THE TIMECODE WHILE SERATO IS RUNNING? The roadmap lists
// four ways to share the input and ranks "shared WASAPI" second, with the note
// "depends on the driver, to be tested and not presumed". This is that test.
//
// It opens the endpoint in SHARED mode and never in exclusive. That is not a
// convenience: taking a DJ mixer's input exclusively would stop Serato dead, and
// the roadmap calls doing so in follower mode "the mistake not to make".
//
// Recognising timecode rather than just measuring a level matters, because "the
// input is not silent" is true of a microphone picking up the room. A DVS
// control signal is two sinusoids of the SAME frequency about a QUARTER CYCLE
// apart -- that quadrature is what encodes direction, and it is what nothing
// else on a mixer's input looks like.
//
// Usage:
//   audio_probe                    list the capture endpoints
//   audio_probe ELITE [seconds]    listen to one
// windows.h defines min and max as macros, which then swallow std::max below.
#define NOMINMAX
#include <windows.h>

// initguid BEFORE the property-key header, or DEFINE_PROPERTYKEY expands to a
// declaration with no type and every PKEY_ below is undefined. ksmedia carries
// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, which the mix format is compared against.
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

// The dominant frequency of a channel, by counting zero crossings over the
// captured block. Cruder than an FFT and entirely sufficient here: a timecode
// carrier is a strong single tone, and what is being asked is "is this the
// carrier" rather than "what is the spectrum".
double dominant_hz(const std::vector<float>& samples, double rate) {
    if (samples.size() < 4) return 0.0;
    int crossings = 0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i - 1] < 0.0f) != (samples[i] < 0.0f)) ++crossings;
    }
    const double seconds = static_cast<double>(samples.size()) / rate;
    return crossings / (2.0 * seconds);
}

// The phase of `samples` at `hz`, by one Goertzel-style projection onto a sine
// and a cosine. The DIFFERENCE between the two channels' phases is the number
// that identifies timecode, and it is also the number a decoder uses to tell
// forwards from backwards.
double phase_at(const std::vector<float>& samples, double hz, double rate) {
    double real = 0.0, imaginary = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const double angle = 2.0 * kPi * hz * static_cast<double>(i) / rate;
        real += samples[i] * std::cos(angle);
        imaginary += samples[i] * std::sin(angle);
    }
    return std::atan2(imaginary, real);
}

double peak_of(const std::vector<float>& samples) {
    double peak = 0.0;
    for (float sample : samples) peak = std::max(peak, std::fabs(static_cast<double>(sample)));
    return peak;
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
        std::printf("%u entrees de capture :\n", count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* device = nullptr;
            collection->Item(i, &device);
            std::printf("  %u : %s\n", i, endpoint_name(device).c_str());
            device->Release();
        }
        std::printf("\nusage : audio_probe <nom ou index> [secondes]\n");
        return 0;
    }

    // By name fragment, so "ELITE" finds "Entrée ligne (Reloop ELITE ...)".
    IMMDevice* device = nullptr;
    std::string chosen_name;
    const std::string wanted = argv[1];
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* candidate = nullptr;
        collection->Item(i, &candidate);
        std::string name = endpoint_name(candidate);
        std::string lower_name = name, lower_want = wanted;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        std::transform(lower_want.begin(), lower_want.end(), lower_want.begin(), ::tolower);
        if (lower_name.find(lower_want) != std::string::npos) {
            device = candidate;
            chosen_name = name;
            break;
        }
        candidate->Release();
    }
    if (device == nullptr) {
        std::fprintf(stderr, "aucune entree ne correspond a \"%s\"\n", argv[1]);
        return 1;
    }
    const double seconds = argc > 2 ? std::stod(argv[2]) : 4.0;

    IAudioClient* client = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&client)))) {
        std::fprintf(stderr, "activation impossible\n");
        return 1;
    }

    WAVEFORMATEX* format = nullptr;
    client->GetMixFormat(&format);

    // SHARED, never exclusive. Exclusive would lock the mixer's input away from
    // Serato, which is the one thing follower mode must never do.
    const REFERENCE_TIME buffer = 10'000'000 / 10;  // 100 ms
    HRESULT opened = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer, 0, format,
                                       nullptr);
    if (FAILED(opened)) {
        std::fprintf(stderr,
                     "Initialize a echoue (0x%08lX). Un autre logiciel tient peut-etre "
                     "l'entree en exclusif.\n",
                     static_cast<unsigned long>(opened));
        return 1;
    }

    IAudioCaptureClient* capture = nullptr;
    client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture));
    client->Start();

    const double rate = format->nSamplesPerSec;
    const unsigned channels = format->nChannels;
    const bool is_float = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                          (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                           reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat ==
                               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    std::printf("\"%s\"\n", chosen_name.c_str());
    std::printf("%.0f Hz, %u canaux, %u bits, %s, mode PARTAGE\n", rate, channels,
                format->wBitsPerSample, is_float ? "flottant" : "entier");
    std::printf("capture de %.1f s...\n\n", seconds);
    std::fflush(stdout);

    std::vector<std::vector<float>> lanes(channels);
    const auto deadline = GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0);
    while (GetTickCount64() < deadline) {
        UINT32 available = 0;
        capture->GetNextPacketSize(&available);
        while (available > 0) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0 && data != nullptr) {
                for (UINT32 f = 0; f < frames; ++f) {
                    for (unsigned c = 0; c < channels; ++c) {
                        float value = 0.0f;
                        if (is_float) {
                            value = reinterpret_cast<const float*>(data)[f * channels + c];
                        } else if (format->wBitsPerSample == 16) {
                            value = reinterpret_cast<const std::int16_t*>(
                                        data)[f * channels + c] /
                                    32768.0f;
                        }
                        lanes[c].push_back(value);
                    }
                }
            } else {
                // A silent packet still advances time; recording zeros keeps the
                // phase estimate honest instead of splicing over the gap.
                for (UINT32 f = 0; f < frames; ++f) {
                    for (unsigned c = 0; c < channels; ++c) lanes[c].push_back(0.0f);
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

    std::printf("%-10s %10s %12s\n", "canal", "crete", "frequence");
    for (unsigned c = 0; c < channels; ++c) {
        std::printf("%-10u %10.4f %9.1f Hz\n", c, peak_of(lanes[c]),
                    dominant_hz(lanes[c], rate));
    }
    std::printf("\n");

    const double peak_l = peak_of(lanes[0]);
    const double peak_r = channels > 1 ? peak_of(lanes[1]) : 0.0;

    if (peak_l < 0.001 && peak_r < 0.001) {
        std::printf("=> ENTREE SILENCIEUSE.\n");
        std::printf("   Soit rien n'est branche dessus, soit le timecode ne passe pas\n");
        std::printf("   par cette sortie USB. Verifie qu'une platine joue le disque\n");
        std::printf("   de controle et que la voie est en PHONO.\n");
        return 3;
    }

    if (channels < 2) {
        std::printf("=> UN SEUL CANAL : impossible de reconnaitre du timecode.\n");
        std::printf("   Le sens de lecture est porte par la PHASE ENTRE LES DEUX voies.\n");
        return 3;
    }

    const double hz_l = dominant_hz(lanes[0], rate);
    const double hz_r = dominant_hz(lanes[1], rate);
    const double carrier = 0.5 * (hz_l + hz_r);
    double difference = phase_at(lanes[1], carrier, rate) - phase_at(lanes[0], carrier, rate);
    while (difference > kPi) difference -= 2.0 * kPi;
    while (difference < -kPi) difference += 2.0 * kPi;
    const double degrees = difference * 180.0 / kPi;

    std::printf("porteuse ~%.1f Hz, dephasage L/R %.1f\xC2\xB0\n", carrier, degrees);

    // A quarter cycle either way is the signature. The SIGN of it is the
    // direction the record is turning, which is the entire trick DVS rests on.
    const bool quadrature = std::fabs(std::fabs(degrees) - 90.0) < 35.0;
    const bool plausible_carrier = carrier > 500.0 && carrier < 3000.0;

    if (quadrature && plausible_carrier) {
        std::printf("\n=> C'EST DU TIMECODE, ET IL ARRIVE EN WASAPI PARTAGE.\n");
        std::printf("   Le sens : %s.\n", degrees > 0 ? "avant" : "arriere");
        std::printf("   La voie \"WASAPI partage\" du roadmap est donc ouverte : l'app\n");
        std::printf("   peut lire le timecode sans prendre le peripherique a Serato.\n");
    } else if (plausible_carrier) {
        std::printf("\n=> Un ton a %.0f Hz, mais sans quadrature nette (%.0f\xC2\xB0).\n",
                    carrier, degrees);
        std::printf("   Possible si le disque est a l'arret, si une seule voie est\n");
        std::printf("   branchee, ou si l'entree somme les deux canaux.\n");
    } else {
        std::printf("\n=> Du signal, mais pas une porteuse de timecode.\n");
        std::printf("   C'est probablement de la musique : cette sortie USB porte le\n");
        std::printf("   MIX, pas les entrees phono.\n");
    }

    capture->Release();
    client->Release();
    device->Release();
    if (collection != nullptr) collection->Release();
    enumerator->Release();
    CoTaskMemFree(format);
    CoUninitialize();
    return 0;
}
