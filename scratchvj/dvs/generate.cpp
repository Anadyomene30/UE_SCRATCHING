#include "dvs/generate.h"

#include <algorithm>
#include <cmath>

extern "C" {
#include "timecoder.h"
}

namespace svj::dvs {
namespace {

constexpr double kPi = 3.14159265358979323846;

// The LFSR xwax uses, copied from mktimecode.c so the bitstream is identical.
// `taps | 1` matches timecoder.c's fwd(), which always includes the end tap.
using bits_t = std::uint32_t;

bits_t lfsr(bits_t code, bits_t taps) {
    bits_t taken = code & taps;
    bits_t xrs = 0;
    while (taken != 0) {
        xrs += taken & 0x1;
        taken >>= 1;
    }
    return xrs & 0x1;
}

bits_t fwd(bits_t current, bits_t taps, unsigned bits) {
    const bits_t l = lfsr(current, taps | 0x1);
    return (current >> 1) | (l << (bits - 1));
}

}  // namespace

bool generate_timecode(const GeneratorSpec& spec, std::size_t frames,
                       std::vector<std::int16_t>& out) {
    timecode_def* def = timecoder_find_definition(spec.definition.c_str());
    if (def == nullptr || spec.sample_rate < 8000 || spec.pitch <= 0.0) return false;

    const auto bits = static_cast<unsigned>(def->bits);
    const double resolution = def->resolution;  // carrier cycles per second

    // Wind the LFSR forward to the requested position. One step per carrier
    // cycle, and a position in seconds is `resolution` cycles per second -- the
    // same conversion timecoder_get_resolution() undoes on the way back.
    auto b = static_cast<bits_t>(def->seed);
    const auto start_cycle =
        static_cast<long long>(std::llround(spec.position_s * resolution));
    for (long long i = 0; i < start_cycle; ++i) {
        b = fwd(b, static_cast<bits_t>(def->taps), bits);
    }

    out.assign(frames * 2, 0);

    // `cycle` is the carrier phase in whole cycles, counted from the START of
    // this block. The bit advances every time it crosses an integer, exactly as
    // mktimecode advances it, so the modulation stays aligned to the carrier
    // however the block is chopped up.
    long long emitted = 0;
    const double amplitude = std::clamp(spec.amplitude, 0.0, 1.0) * 32767.0;

    for (std::size_t s = 0; s < frames; ++s) {
        const double time = static_cast<double>(s) / spec.sample_rate;
        const double cycle = time * resolution * spec.pitch;
        const double angle = cycle * kPi * 2.0;

        const double x = std::sin(angle);
        const double y = std::cos(angle);

        // A zero bit dips the amplitude once per cycle; a one leaves it alone.
        // The decoder reads the bit by comparing the primary channel's height
        // at the moment the secondary crosses zero, so the DIP is the message.
        const double modulate =
            1.0 - (-std::cos(angle) + 1.0) * 0.25 * ((b & 0x1) == 0 ? 1.0 : 0.0);

        const auto left = static_cast<std::int16_t>(-y * modulate * amplitude);
        const auto right = static_cast<std::int16_t>(x * modulate * amplitude);

        out[s * 2] = spec.swap_channels ? right : left;
        out[s * 2 + 1] = spec.swap_channels ? left : right;

        if (static_cast<long long>(cycle) > emitted) {
            b = fwd(b, static_cast<bits_t>(def->taps), bits);
            emitted = static_cast<long long>(cycle);
        }
    }
    return true;
}

}  // namespace svj::dvs
