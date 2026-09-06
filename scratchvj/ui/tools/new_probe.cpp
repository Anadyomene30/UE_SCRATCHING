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
