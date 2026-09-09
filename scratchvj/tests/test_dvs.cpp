#include <cmath>
#include <vector>

#include "dvs/decoder.h"
#include "dvs/generate.h"
#include "harness.h"

using namespace svj;
using namespace svj::dvs;

namespace {

constexpr unsigned kRate = 48000;

// Feeds `seconds` of generated timecode in realistic block sizes and returns the
// last sample the decoder produced. Blocks are deliberately not a round number
// of carrier cycles: an audio callback never hands over a tidy amount.
DecoderSample decode_generated(const GeneratorSpec& spec, double seconds) {
    TimecodeDecoder decoder;
    CHECK(decoder.open(spec.definition, kRate));

    const auto total = static_cast<std::size_t>(kRate * seconds);
    std::vector<std::int16_t> pcm;
    CHECK(generate_timecode(spec, total, pcm));

    DecoderSample last;
    const std::size_t block = 233;
    for (std::size_t i = 0; i < total; i += block) {
        const std::size_t count = std::min(block, total - i);
        last = decoder.submit(pcm.data() + i * 2, count,
                              static_cast<double>(i) / kRate);
    }
    return last;
}

}  // namespace

// The load-bearing test: a position put into the signal comes back out of it.
// Everything else about the DVS layer is plumbing around this one property.
SVJ_TEST("dvs: a generated position decodes back to itself") {
    GeneratorSpec spec;
    spec.position_s = 30.0;

    const DecoderSample sample = decode_generated(spec, 2.0);
    CHECK(sample.position_s >= 0.0);  // locked at all
    // Two seconds of signal were fed from position 30, so the decoder should be
    // near 32. Tolerance is generous because the lock takes a fraction of a
    // second to acquire; what matters is that it is THERE and not at 0 or 30.
    CHECK_NEAR(sample.position_s, 32.0, 0.25);
}

SVJ_TEST("dvs: a different position decodes to that position") {
    // Guards against a decoder that "locks" on something constant -- a bug that
    // the single-position test above would pass happily.
    GeneratorSpec spec;
    spec.position_s = 300.0;
    const DecoderSample sample = decode_generated(spec, 2.0);
    CHECK(sample.position_s >= 0.0);
    CHECK_NEAR(sample.position_s, 302.0, 0.25);
}

SVJ_TEST("dvs: nominal speed reads as a pitch of one") {
    GeneratorSpec spec;
    spec.position_s = 60.0;
    const DecoderSample sample = decode_generated(spec, 2.0);
    CHECK_NEAR(sample.pitch, 1.0f, 0.05f);
}

SVJ_TEST("dvs: a slower record reads as a lower pitch") {
    GeneratorSpec spec;
    spec.position_s = 60.0;
    spec.pitch = 0.5;
    const DecoderSample sample = decode_generated(spec, 2.0);
    CHECK_NEAR(sample.pitch, 0.5f, 0.05f);
    // Half speed for two seconds covers one second of record.
    CHECK(sample.position_s >= 0.0);
    CHECK_NEAR(sample.position_s, 61.0, 0.25);
}

SVJ_TEST("dvs: silence never claims a lock") {
    // The fault that would be worst on stage: a decoder inventing a position
    // when the needle is up. core/timecode freezes on a negative position, so
    // this is what makes freezing possible.
    TimecodeDecoder decoder;
    CHECK(decoder.open("serato_2a", kRate));

    const std::vector<std::int16_t> quiet(48000 * 2, 0);
    DecoderSample last;
    for (std::size_t i = 0; i < 48000; i += 512) {
        last = decoder.submit(quiet.data() + i * 2, std::min<std::size_t>(512, 48000 - i),
                              static_cast<double>(i) / kRate);
    }
    CHECK(last.position_s < 0.0);
    CHECK_NEAR(last.signal_level, 0.0f, 1e-6f);
}

SVJ_TEST("dvs: noise never claims a lock") {
    TimecodeDecoder decoder;
    CHECK(decoder.open("serato_2a", kRate));

    // Deterministic pseudo-noise, so this says the same thing on every machine.
    std::vector<std::int16_t> noise(48000 * 2);
    std::uint32_t seed = 987654321;
    for (std::int16_t& sample : noise) {
        seed = seed * 1664525u + 1013904223u;
        sample = static_cast<std::int16_t>((seed >> 16) & 0x7FFF) - 16384;
    }

    DecoderSample last;
    for (std::size_t i = 0; i < 48000; i += 512) {
        last = decoder.submit(noise.data() + i * 2, std::min<std::size_t>(512, 48000 - i),
                              static_cast<double>(i) / kRate);
    }
    CHECK(last.position_s < 0.0);
}

SVJ_TEST("dvs: the signal level follows the amplitude fed in") {
    GeneratorSpec loud;
    loud.position_s = 10.0;
    loud.amplitude = 0.9;
    GeneratorSpec quiet = loud;
    quiet.amplitude = 0.05;

    CHECK(decode_generated(loud, 1.0).signal_level >
          decode_generated(quiet, 1.0).signal_level + 0.5f);
}

SVJ_TEST("dvs: a weak cartridge still locks") {
    // A quiet signal must not be mistaken for no signal. The level floor lives
    // in core/timecode's confidence, not here, so the decoder itself has to
    // cope with a small one.
    GeneratorSpec spec;
    spec.position_s = 45.0;
    spec.amplitude = 0.05;
    const DecoderSample sample = decode_generated(spec, 2.0);
    CHECK(sample.position_s >= 0.0);
    CHECK_NEAR(sample.position_s, 47.0, 0.25);
}

SVJ_TEST("dvs: swapping the channels reverses the apparent direction") {
    // The calibration fact, stated as a test. A reversed RCA pair makes a record
    // played forwards look like one played backwards, and NOTHING downstream can
    // tell -- which is why tools/dvs_check calibrates it against real gear
    // rather than this file asserting which way round is correct.
    GeneratorSpec straight;
    straight.position_s = 60.0;
    GeneratorSpec swapped = straight;
    swapped.swap_channels = true;

    const float forward = decode_generated(straight, 1.5).pitch;
    const float reversed = decode_generated(swapped, 1.5).pitch;
    CHECK(forward > 0.5f);
    CHECK(reversed < -0.5f);
}

SVJ_TEST("dvs: an unknown definition is refused rather than guessed") {
    TimecodeDecoder decoder;
    CHECK(!decoder.open("not_a_real_timecode", kRate));
    CHECK(!decoder.ready());

    std::vector<std::int16_t> pcm;
    GeneratorSpec spec;
    spec.definition = "not_a_real_timecode";
    CHECK(!generate_timecode(spec, 128, pcm));
}

SVJ_TEST("dvs: every definition xwax advertises can be opened") {
    // known_definitions() is a hand-copied list, because xwax does not export
    // its table. This is what stops it silently drifting from the real one.
    for (const std::string& name : known_definitions()) {
        TimecodeDecoder decoder;
        CHECK(decoder.open(name, kRate));
    }
}

SVJ_TEST("dvs: a float source decodes the same as the integer one") {
    // The app feeds WASAPI floats; the tests above feed integers. If the two
    // paths disagreed, everything verified here would be verifying the wrong
    // path.
    GeneratorSpec spec;
    spec.position_s = 90.0;

    const auto total = static_cast<std::size_t>(kRate * 2.0);
    std::vector<std::int16_t> pcm;
    CHECK(generate_timecode(spec, total, pcm));

    std::vector<float> as_float(pcm.size());
    for (std::size_t i = 0; i < pcm.size(); ++i) {
        as_float[i] = static_cast<float>(pcm[i]) / 32767.0f;
    }

    TimecodeDecoder a, b;
    CHECK(a.open(spec.definition, kRate));
    CHECK(b.open(spec.definition, kRate));

    DecoderSample from_int, from_float;
    const std::size_t block = 233;
    for (std::size_t i = 0; i < total; i += block) {
        const std::size_t count = std::min(block, total - i);
        const double now = static_cast<double>(i) / kRate;
        from_int = a.submit(pcm.data() + i * 2, count, now);
        from_float = b.submit(as_float.data() + i * 2, count, now);
    }
    CHECK(from_int.position_s >= 0.0);
    CHECK_NEAR(from_float.position_s, from_int.position_s, 0.01);
}
