// scratchvj — what actually arrives on an audio input, and whether it is timecode.
//
// This settles the second of the two questions the roadmap leaves to the
// hardware, and it settles the one the whole audio architecture rests on: CAN
// THIS APPLICATION READ THE TIMECODE WHILE SERATO IS RUNNING? The roadmap lists
// four ways to share the input and ranks "shared WASAPI" third, with the note
// "depends on the driver, to be tested and not presumed". This is that test.
//
// It opens every endpoint in SHARED mode and never in exclusive. That is not a
// convenience: taking a DJ mixer's input exclusively would stop Serato dead, and
// the roadmap calls doing so in follower mode "the mistake not to make". A
// refusal is itself a finding -- it means something else already holds the
// endpoint.
//
// WITH AN MWM PHASE THERE IS NO CONTROL VINYL. The dock SYNTHESISES the timecode
// from the remotes' motion and puts it out on RCA, so a signal exists only while
// a platter is actually turning. That is why `all` sweeps every input in one go:
// the routing is not obvious on a desk with several interfaces, and asking
// someone to spin a record once per guess is not a test, it is a chore.
//
// Recognising timecode rather than measuring a level matters, because "the input
// is not silent" is true of a microphone picking up the room. A DVS control
// signal is two sinusoids of the SAME frequency about a QUARTER CYCLE apart --
// that quadrature is what encodes direction, and it is what nothing else on a
// mixer's input looks like.
//
// Usage:
//   audio_probe                    list the capture endpoints
//   audio_probe all [seconds]      sweep them all, seconds EACH
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
#include <utility>
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

// The dominant frequency of a channel, by counting zero crossings. Cruder than
// an FFT and entirely sufficient here: a timecode carrier is a strong single
// tone, and the question is "is this the carrier", not "what is the spectrum".
double dominant_hz(const std::vector<float>& samples, double rate) {
    if (samples.size() < 4) return 0.0;
    int crossings = 0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i - 1] < 0.0f) != (samples[i] < 0.0f)) ++crossings;
    }
    const double seconds = static_cast<double>(samples.size()) / rate;
    return seconds > 0.0 ? crossings / (2.0 * seconds) : 0.0;
}

// One Goertzel-style projection onto `hz`, giving both the phase and how much of
// the signal actually sits there.
struct Projection {
    double phase = 0.0;
    // Magnitude at `hz` relative to the signal's own energy. A pure sine scores
    // about 0.71 whatever its level; broadband noise scores about 1/sqrt(N).
    //
    // This is the check that matters, and it was added because without it the
    // probe called a webcam picking up a quiet room "TIMECODE": on noise the
    // frequency and phase estimates are essentially random, so roughly one pair
    // in four lands near the +/-90 degrees quadrature looks for. Level alone
    // does not save you either -- the room was at 0.012, a plausible level for
    // a weak turntable.
    double tonality = 0.0;
};

Projection project(const std::vector<float>& samples, double hz, double rate) {
    Projection out;
    if (samples.empty() || hz <= 0.0) return out;

    double real = 0.0, imaginary = 0.0, energy = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const double angle = 2.0 * kPi * hz * static_cast<double>(i) / rate;
        real += samples[i] * std::cos(angle);
        imaginary += samples[i] * std::sin(angle);
        energy += static_cast<double>(samples[i]) * samples[i];
    }
    out.phase = std::atan2(imaginary, real);

    const double n = static_cast<double>(samples.size());
    const double rms = std::sqrt(energy / n);
    if (rms > 1e-9) out.tonality = std::sqrt(real * real + imaginary * imaginary) / (rms * n);
    return out;
}

double peak_of(const std::vector<float>& samples) {
    double peak = 0.0;
    for (float sample : samples) {
        peak = std::max(peak, std::fabs(static_cast<double>(sample)));
    }
    return peak;
}

// What one endpoint had to say.
struct Reading {
    bool opened = false;
    std::string failure;
    double rate = 0.0;
    unsigned channels = 0;
    double peak = 0.0;
    double carrier_hz = 0.0;
    double phase_deg = 0.0;
    bool quadrature = false;
    bool plausible_carrier = false;
    double tonality = 0.0;
    // WHICH PAIR carried it. A multichannel interface -- the MOTU here reports
    // 24 channels on one endpoint -- can have the timecode on any adjacent pair,
    // and checking only 0/1 would report a confident "silence" for a signal
    // sitting on inputs 7 and 8. That is the false negative that costs an hour
    // of unplugging things.
    unsigned pair = 0;
};

Reading listen(IMMDevice* device, double seconds) {
    Reading reading;

    IAudioClient* client = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&client)))) {
        reading.failure = "activation impossible";
        return reading;
    }

    // GetMixFormat can fail on a pro driver whose WDM side is half-present, and
    // its output must not then be handed to Initialize -- a null format is what
    // turns into the misleading 0x80070006 (ERROR_INVALID_HANDLE) below.
    WAVEFORMATEX* format = nullptr;
    const HRESULT got_format = client->GetMixFormat(&format);
    if (FAILED(got_format) || format == nullptr) {
        char message[96];
        std::snprintf(message, sizeof(message), "pas de format mixe (0x%08lX)",
                      static_cast<unsigned long>(got_format));
        reading.failure = message;
        client->Release();
        return reading;
    }

    const REFERENCE_TIME buffer = 1000000;  // 100 ms, in 100 ns units
    const HRESULT opened =
        client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer, 0, format, nullptr);
    if (FAILED(opened)) {
        // Named, not guessed. The three that matter here are distinct causes,
        // and calling ERROR_INVALID_HANDLE "exclusive" sent me chasing the wrong
        // thing for a while.
        const char* why = "cause inconnue";
        switch (static_cast<unsigned long>(opened)) {
            case 0x88890008: why = "format non supporte en partage"; break;   // UNSUPPORTED_FORMAT
            case 0x8889000A: why = "peripherique deja pris"; break;           // DEVICE_IN_USE
            case 0x8889000B: why = "exclusif refuse"; break;                  // EXCLUSIVE_MODE_NOT_ALLOWED
            case 0x88890004: why = "peripherique invalide/debranche"; break;  // DEVICE_INVALIDATED
            case 0x80070006: why = "handle invalide (pilote WDM incomplet ?)"; break;
            default: break;
        }
        char message[128];
        std::snprintf(message, sizeof(message), "refuse (0x%08lX) -- %s",
                      static_cast<unsigned long>(opened), why);
        reading.failure = message;
        CoTaskMemFree(format);
        client->Release();
        return reading;
    }

    IAudioCaptureClient* capture = nullptr;
    client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture));
    client->Start();

    reading.opened = true;
    reading.rate = format->nSamplesPerSec;
    reading.channels = format->nChannels;
    const bool is_float = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                          (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                           reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat ==
                               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    std::vector<std::vector<float>> lanes(reading.channels);
    const ULONGLONG deadline =
        GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0);
    while (GetTickCount64() < deadline) {
        UINT32 available = 0;
        capture->GetNextPacketSize(&available);
        while (available > 0) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            const bool silent =
                (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
            for (UINT32 f = 0; f < frames; ++f) {
                for (unsigned c = 0; c < reading.channels; ++c) {
                    float value = 0.0f;
                    if (!silent) {
                        if (is_float) {
                            value = reinterpret_cast<const float*>(
                                data)[f * reading.channels + c];
                        } else if (format->wBitsPerSample == 16) {
                            value = reinterpret_cast<const std::int16_t*>(
                                        data)[f * reading.channels + c] /
                                    32768.0f;
                        }
                    }
                    // A silent packet still advances time, so its zeros are
                    // recorded rather than skipped: splicing over the gap would
                    // corrupt the phase estimate below.
                    lanes[c].push_back(value);
                }
            }
            capture->ReleaseBuffer(frames);
            capture->GetNextPacketSize(&available);
        }
        Sleep(5);
    }
    client->Stop();

    if (!lanes.empty() && !lanes[0].empty()) {
        for (const std::vector<float>& lane : lanes) {
            reading.peak = std::max(reading.peak, peak_of(lane));
        }

        // Every adjacent pair, not just the first. The best-scoring one wins,
        // where "best" prefers an actual timecode signature over a merely loud
        // pair -- a 24-input interface usually has something on SOME channel,
        // and reporting the loudest would bury the answer under the room mic.
        double best_score = -1.0;
        for (unsigned c = 0; c + 1 < reading.channels; c += 2) {
            const double pair_peak = std::max(peak_of(lanes[c]), peak_of(lanes[c + 1]));
            if (pair_peak < 0.001) continue;

            const double left = dominant_hz(lanes[c], reading.rate);
            const double right = dominant_hz(lanes[c + 1], reading.rate);
            const double carrier = 0.5 * (left + right);

            const Projection a = project(lanes[c], carrier, reading.rate);
            const Projection b = project(lanes[c + 1], carrier, reading.rate);
            double difference = b.phase - a.phase;
            while (difference > kPi) difference -= 2.0 * kPi;
            while (difference < -kPi) difference += 2.0 * kPi;
            const double degrees = difference * 180.0 / kPi;
            const double tonality = std::min(a.tonality, b.tonality);

            // Three conditions, and all three are needed. A quarter cycle either
            // way is the signature, and its SIGN is the direction the platter
            // turns -- but on noise the phase is random, so quadrature alone
            // fires about one time in four. Tonality is what says "this is one
            // tone and not a room". The level floor is deliberately low: a
            // weak cartridge is a real case, and tonality already rejects the
            // quiet noise that floor would otherwise have to.
            const bool quadrature = std::fabs(std::fabs(degrees) - 90.0) < 35.0;
            const bool plausible = carrier > 500.0 && carrier < 3000.0;
            const bool tonal = tonality > 0.30 && pair_peak > 0.005;

            // A pair that looks like timecode beats any pair that does not,
            // whatever their levels; among equals, the more tonal one.
            const double score =
                (quadrature && plausible && tonal ? 1000.0 : 0.0) + tonality;
            if (score > best_score) {
                best_score = score;
                reading.pair = c;
                reading.carrier_hz = carrier;
                reading.phase_deg = degrees;
                reading.tonality = tonality;
                reading.quadrature = quadrature;
                reading.plausible_carrier = plausible && tonal;
            }
        }
    }

    capture->Release();
    CoTaskMemFree(format);
    client->Release();
    return reading;
}

// Does a pair look like timecode? The one decision the whole tool exists to
// make, factored out so it can be exercised on signals built here rather than
// only on whatever happens to be plugged in.
struct Verdict {
    double carrier_hz = 0.0;
    double phase_deg = 0.0;
    double tonality = 0.0;
    // How much the carrier's amplitude varies from cycle to cycle. THIS is the
    // data: a DVS control signal encodes its position by dipping the carrier on
    // every zero bit, so a real one swings by tens of percent. A bare carrier
    // sits flat, and carries direction and speed but no position at all.
    //
    // Added because the probe called a bare 1000 Hz quadrature tone "TIMECODE":
    // an MWM Phase in HID mode emits exactly that on its RCA outputs, and
    // quadrature alone cannot tell it from the real thing. The decoder could
    // then never lock, and nothing said why.
    double modulation = 0.0;
    bool is_timecode = false;
};

// Peak amplitude per carrier cycle, as a relative spread. ~0 for a bare tone,
// tens of percent for a modulated one.
double modulation_depth(const std::vector<float>& samples, double hz, double rate) {
    if (hz <= 0.0 || samples.empty()) return 0.0;
    const double period = rate / hz;
    if (period < 4.0) return 0.0;

    std::vector<double> peaks;
    for (double at = 0.0; at + period < static_cast<double>(samples.size());
         at += period) {
        double peak = 0.0;
        const auto first = static_cast<std::size_t>(at);
        const auto last = static_cast<std::size_t>(at + period);
        for (std::size_t i = first; i < last; ++i) {
            peak = std::max(peak, std::fabs(static_cast<double>(samples[i])));
        }
        peaks.push_back(peak);
    }
    if (peaks.size() < 8) return 0.0;

    double mean = 0.0;
    for (double p : peaks) mean += p;
    mean /= static_cast<double>(peaks.size());
    if (mean < 1e-9) return 0.0;

    double variance = 0.0;
    for (double p : peaks) variance += (p - mean) * (p - mean);
    return std::sqrt(variance / static_cast<double>(peaks.size())) / mean;
}

Verdict judge(const std::vector<float>& left, const std::vector<float>& right,
              double rate) {
    Verdict verdict;
    const double peak = std::max(peak_of(left), peak_of(right));
    verdict.carrier_hz = 0.5 * (dominant_hz(left, rate) + dominant_hz(right, rate));

    const Projection a = project(left, verdict.carrier_hz, rate);
    const Projection b = project(right, verdict.carrier_hz, rate);
    double difference = b.phase - a.phase;
    while (difference > kPi) difference -= 2.0 * kPi;
    while (difference < -kPi) difference += 2.0 * kPi;
    verdict.phase_deg = difference * 180.0 / kPi;
    verdict.tonality = std::min(a.tonality, b.tonality);

    // Three conditions, and all three are needed. A quarter cycle either way is
    // the signature, and its SIGN is the direction the platter turns -- but on
    // noise the phase is random, so quadrature alone fires about one time in
    // four. Tonality is what says "this is one tone and not a room". The level
    // floor is deliberately low: a weak cartridge is a real case, and tonality
    // already rejects the quiet noise that a high floor would have to.
    const bool quadrature = std::fabs(std::fabs(verdict.phase_deg) - 90.0) < 35.0;
    const bool plausible = verdict.carrier_hz > 500.0 && verdict.carrier_hz < 3000.0;
    const bool tonal = verdict.tonality > 0.30 && peak > 0.005;

    verdict.modulation =
        std::max(modulation_depth(left, verdict.carrier_hz, rate),
                 modulation_depth(right, verdict.carrier_hz, rate));
    // 5% is well above the ~0.2% a bare carrier shows and well below the tens of
    // percent a real bitstream produces, so it separates them without being
    // fussy about a noisy cartridge.
    const bool carries_data = verdict.modulation > 0.05;

    verdict.is_timecode = quadrature && plausible && tonal && carries_data;
    return verdict;
}

// The falsification the probe needed. It called a webcam picking up a quiet room
// "TIMECODE" once, and no amount of care with real hardware would have caught
// that reliably -- the room has to be noisy in the right way on the right day.
// Built signals catch it every time.
int self_test() {
    constexpr double kRate = 48000.0;
    const std::size_t n = static_cast<std::size_t>(kRate);  // one second
    int failures = 0;

    const auto report = [&](const char* name, const Verdict& verdict, bool expected) {
        std::printf("%-34s %7.0f Hz  %+6.0f deg  ton %.2f  mod %.3f  -> %s\n", name,
                    verdict.carrier_hz, verdict.phase_deg, verdict.tonality,
                    verdict.modulation, verdict.is_timecode ? "TIMECODE" : "rejete");
        if (verdict.is_timecode != expected) {
            std::fprintf(stderr, "   ATTENDU : %s\n", expected ? "TIMECODE" : "rejete");
            ++failures;
        }
    };

    // A control signal: two sines a quarter cycle apart, with the amplitude
    // dipping on alternate cycles the way a bitstream modulates it.
    const auto fill = [&](std::vector<float>& left, std::vector<float>& right,
                          double hz, double amplitude, double phase, bool modulated) {
        for (std::size_t i = 0; i < n; ++i) {
            const double t = 2.0 * kPi * hz * static_cast<double>(i) / kRate;
            // Every other carrier cycle carries a "zero bit" and is quieter.
            const auto cycle = static_cast<long long>(hz * static_cast<double>(i) / kRate);
            const double depth = (modulated && (cycle & 1) == 0) ? 0.55 : 1.0;
            left[i] = static_cast<float>(amplitude * depth * std::sin(t));
            right[i] = static_cast<float>(amplitude * depth * std::sin(t + phase));
        }
    };

    {
        std::vector<float> left(n), right(n);
        fill(left, right, 1000.0, 0.4, kPi * 0.5, true);
        const Verdict one_way = judge(left, right, kRate);
        report("timecode 1 kHz, R en avance", one_way, true);

        fill(left, right, 1000.0, 0.4, -kPi * 0.5, true);
        const Verdict other_way = judge(left, right, kRate);
        report("timecode 1 kHz, R en retard", other_way, true);

        // The two directions must land on OPPOSITE signs -- that is what makes
        // the phase usable as a direction at all. WHICH sign means forwards is
        // NOT asserted, because it cannot be known here: it depends on the
        // dock's convention and on which way the RCA pair is wired, and a guess
        // would be the "inverted 360 yaw" defect over again. It is a one-line
        // calibration against the real gear, and until that is done the tool
        // reports the sign without naming a direction.
        if ((one_way.phase_deg > 0.0) == (other_way.phase_deg > 0.0)) {
            std::fprintf(stderr, "   les deux sens donnent le meme signe : inutilisable\n");
            ++failures;
        }
    }

    // A weak carrier: level must not be what disqualifies it.
    {
        std::vector<float> left(n), right(n);
        fill(left, right, 1200.0, 0.02, kPi * 0.5, true);
        report("timecode faible (0.02)", judge(left, right, kRate), true);
    }

    // THE CASE THAT WAS MISSING, and it cost a real debugging session. An MWM
    // Phase in HID mode puts a bare 1 kHz quadrature tone on its RCA outputs:
    // textbook quadrature, perfectly tonal, and carrying no position at all,
    // because the bitstream that encodes position is exactly the amplitude
    // modulation this signal does not have. Calling it timecode sent the
    // decoder hunting for a lock that could never come.
    {
        std::vector<float> left(n), right(n);
        fill(left, right, 1000.0, 0.2, kPi * 0.5, false);
        report("porteuse nue (Phase en HID)", judge(left, right, kRate), false);
    }

    // The room that fooled it. Deterministic pseudo-noise, so this test says the
    // same thing on every machine and every run.
    {
        std::vector<float> left(n), right(n);
        std::uint32_t seed = 12345;
        const auto next = [&seed]() {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>(
                (static_cast<double>(seed >> 8) / 8388608.0 - 1.0) * 0.05);
        };
        for (std::size_t i = 0; i < n; ++i) {
            left[i] = next();
            right[i] = next();
        }
        report("bruit de piece", judge(left, right, kRate), false);
    }

    // Music: correlated, loud, but not one tone.
    {
        std::vector<float> left(n), right(n);
        for (std::size_t i = 0; i < n; ++i) {
            const double t = static_cast<double>(i) / kRate;
            const double mix = 0.3 * std::sin(2.0 * kPi * 220.0 * t) +
                               0.2 * std::sin(2.0 * kPi * 440.0 * t) +
                               0.2 * std::sin(2.0 * kPi * 1330.0 * t);
            left[i] = static_cast<float>(mix);
            right[i] = static_cast<float>(mix * 0.9);
        }
        report("musique", judge(left, right, kRate), false);
    }

    // Silence must not be a carrier either.
    {
        const std::vector<float> quiet(n, 0.0f);
        report("silence", judge(quiet, quiet, kRate), false);
    }

    std::printf("\n");
    if (failures > 0) {
        std::fprintf(stderr, "%d cas mal juges : le detecteur n'est pas fiable\n", failures);
        return 2;
    }
    std::printf("detecteur conforme : timecode reconnu, bruit et musique rejetes\n");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "selftest") return self_test();

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
        std::printf("\nusage : audio_probe <nom | index | all> [secondes]\n");
        return 0;
    }

    const std::string wanted = argv[1];
    const double seconds = argc > 2 ? std::stod(argv[2]) : 4.0;
    const bool scan_all = wanted == "all" || wanted == "tout";

    std::vector<UINT> chosen;
    if (scan_all) {
        for (UINT i = 0; i < count; ++i) chosen.push_back(i);
    } else if (!wanted.empty() &&
               wanted.find_first_not_of("0123456789") == std::string::npos) {
        chosen.push_back(static_cast<UINT>(std::stoul(wanted)));
    } else {
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* candidate = nullptr;
            collection->Item(i, &candidate);
            std::string lower_name = endpoint_name(candidate);
            std::string lower_want = wanted;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                           ::tolower);
            std::transform(lower_want.begin(), lower_want.end(), lower_want.begin(),
                           ::tolower);
            if (lower_name.find(lower_want) != std::string::npos) chosen.push_back(i);
            candidate->Release();
        }
    }
    if (chosen.empty() || chosen.front() >= count) {
        std::fprintf(stderr, "aucune entree ne correspond a \"%s\"\n", argv[1]);
        return 1;
    }

    if (scan_all) {
        // Endpoints are listened to ONE AT A TIME, so the platter has to keep
        // turning for the whole sweep. Sequential rather than simultaneous:
        // opening eleven clients at once is a lot of machinery to get wrong for
        // a probe, and the answer is the same either way.
        std::printf("balayage de %u entrees, %.1f s chacune (~%.0f s au total).\n",
                    static_cast<unsigned>(chosen.size()), seconds,
                    seconds * static_cast<double>(chosen.size()));
        std::printf("FAIS TOURNER LE PLATEAU PENDANT TOUT LE BALAYAGE.\n\n");
        std::fflush(stdout);
    }

    std::vector<std::pair<std::string, Reading>> results;
    for (UINT index : chosen) {
        IMMDevice* device = nullptr;
        collection->Item(index, &device);
        const std::string name = endpoint_name(device);
        std::printf("  %-48s ", name.c_str());
        std::fflush(stdout);

        const Reading reading = listen(device, seconds);
        device->Release();

        if (!reading.opened) {
            std::printf("%s\n", reading.failure.c_str());
        } else if (reading.peak < 0.001) {
            std::printf("silence (%u canaux, %.0f Hz)\n", reading.channels, reading.rate);
        } else {
            std::printf("crete %.3f  voies %u/%u  %.0f Hz  %+.0f deg  tonalite %.2f%s\n", reading.peak, reading.pair + 1, reading.pair + 2, reading.carrier_hz, reading.phase_deg, reading.tonality,
                        (reading.quadrature && reading.plausible_carrier)
                            ? "   <<< TIMECODE"
                            : "");
        }
        std::fflush(stdout);
        results.emplace_back(name, reading);
    }

    // --- the verdict ---------------------------------------------------------
    std::printf("\n");
    const std::pair<std::string, Reading>* found = nullptr;
    for (const std::pair<std::string, Reading>& entry : results) {
        if (entry.second.opened && entry.second.quadrature &&
            entry.second.plausible_carrier) {
            found = &entry;
            break;
        }
    }

    if (found != nullptr) {
        std::printf("=> TIMECODE TROUVE SUR \"%s\", voies %u/%u.\n", found->first.c_str(),
                    found->second.pair + 1, found->second.pair + 2);
        // The sign is reported without being named: which way is forwards
        // depends on the dock's convention and on how the RCA pair is wired, and
        // claiming to know would be the inverted-yaw defect over again. It is
        // one calibration turn of the platter away from being settled.
        std::printf("   porteuse %.0f Hz, dephasage %+.0f deg, tonalite %.2f, en WASAPI\n",
                    found->second.carrier_hz, found->second.phase_deg,
                    found->second.tonality);
        std::printf("   PARTAGE. (Le sens que ce signe designe reste a calibrer en\n");
        std::printf("   tournant le plateau dans un sens connu.)\n");
        std::printf("   La voie \"WASAPI partage\" du roadmap est ouverte : l'app peut\n");
        std::printf("   lire le timecode sans prendre le peripherique a Serato.\n");
        return 0;
    }

    // "Opened but silent" and "could not open at all" are completely different
    // situations, and conflating them sent me chasing the Phase when the real
    // problem was the whole audio graph. If NOTHING opened -- hardware AND the
    // virtual cables that have no cable to unplug -- it is Windows' audio
    // service, not the signal.
    std::size_t opened = 0;
    for (const std::pair<std::string, Reading>& entry : results) {
        if (entry.second.opened) ++opened;
    }
    if (opened == 0) {
        std::printf("=> AUCUNE ENTREE NE S'OUVRE (%zu/%zu en echec).\n", results.size(),
                    results.size());
        std::printf("   Ce n'est pas le Phase : meme les cables virtuels refusent. Le\n");
        std::printf("   graphe audio de Windows est bloque -- typique apres qu'une\n");
        std::printf("   interface pro (MOTU, RME...) a rechange d'horloge. Redemarrer le\n");
        std::printf("   service audio le debloque :\n");
        std::printf("     net stop Audiosrv && net start Audiosrv   (coupe le son en cours)\n");
        return 4;
    }

    bool any_signal = false;
    for (const std::pair<std::string, Reading>& entry : results) {
        if (entry.second.opened && entry.second.peak >= 0.001) any_signal = true;
    }
    if (!any_signal) {
        std::printf("=> TOUTES LES ENTREES OUVERTES SONT SILENCIEUSES.\n");
        std::printf("   Le Phase ne genere du timecode que quand la remote TOURNE, et\n");
        std::printf("   seulement si le dock est appaire et sous tension. Verifie aussi\n");
        std::printf("   que la sortie RCA du dock arrive bien sur une entree de cette\n");
        std::printf("   machine.\n");
        return 3;
    }
    std::printf("=> Du signal, mais aucune porteuse en quadrature.\n");
    std::printf("   Les entrees qui parlent portent probablement le MIX et non les\n");
    std::printf("   entrees phono. Une porteuse de timecode est un ton unique entre\n");
    std::printf("   500 et 3000 Hz avec un dephasage proche de +/-90 deg.\n");
    return 3;
}
