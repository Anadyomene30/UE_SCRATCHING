#include <cmath>
#include <vector>

#include "core/spectrum.h"
#include "harness.h"

using namespace svj;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRate = 48000.0;

// Feeds `seconds` of a sine at `hz`, in blocks of an awkward size on purpose:
// a real audio callback never hands over exactly one window.
void feed_sine(SpectrumAnalyser& analyser, double hz, double seconds,
               double amplitude = 1.0) {
    const auto total = static_cast<std::size_t>(kRate * seconds);
    std::vector<float> block(157);
    std::size_t written = 0;
    while (written < total) {
        const std::size_t count = std::min(block.size(), total - written);
        for (std::size_t i = 0; i < count; ++i) {
            const double t = static_cast<double>(written + i) / kRate;
            block[i] = static_cast<float>(amplitude * std::sin(2.0 * kPi * hz * t));
        }
        analyser.push(block.data(), count);
        written += count;
    }
}

std::size_t band_containing(const SpectrumAnalyser& analyser, double hz) {
    for (std::size_t i = 0; i < analyser.band_count(); ++i) {
        if (hz >= analyser.band_low_hz(i) && hz < analyser.band_high_hz(i)) return i;
    }
    return analyser.band_count();
}

}  // namespace

SVJ_TEST("spectrum: a forward transform followed by its inverse returns the input") {
    // The strongest statement of correctness available without a reference
    // implementation, and it catches a wrong twiddle sign, a bad bit reversal
    // and a missing scale factor all at once.
    std::vector<double> real(64), imaginary(64, 0.0);
    for (std::size_t i = 0; i < real.size(); ++i) {
        real[i] = std::sin(0.3 * static_cast<double>(i)) + 0.4 * static_cast<double>(i % 5);
    }
    const std::vector<double> original = real;

    fft_forward(real, imaginary);
    fft_inverse(real, imaginary);

    for (std::size_t i = 0; i < real.size(); ++i) {
        CHECK_NEAR(real[i], original[i], 1e-9);
        CHECK_NEAR(imaginary[i], 0.0, 1e-9);
    }
}

SVJ_TEST("spectrum: a tone on a bin puts its energy in that bin and its mirror") {
    constexpr std::size_t kN = 64;
    constexpr std::size_t kBin = 7;
    std::vector<double> real(kN), imaginary(kN, 0.0);
    for (std::size_t i = 0; i < kN; ++i) {
        real[i] = std::cos(2.0 * kPi * kBin * static_cast<double>(i) / kN);
    }
    fft_forward(real, imaginary);

    for (std::size_t i = 0; i < kN; ++i) {
        const double magnitude =
            std::sqrt(real[i] * real[i] + imaginary[i] * imaginary[i]);
        // A real signal's spectrum is symmetric, so the energy appears at kBin
        // and at N - kBin.
        const bool expected = (i == kBin || i == kN - kBin);
        if (expected) {
            CHECK_NEAR(magnitude, kN / 2.0, 1e-9);
        } else {
            CHECK_NEAR(magnitude, 0.0, 1e-9);
        }
    }
}

SVJ_TEST("spectrum: a transform of the wrong size is refused rather than corrupting") {
    std::vector<double> real(48, 1.0), imaginary(48, 0.0);
    const std::vector<double> original = real;
    fft_forward(real, imaginary);  // 48 is not a power of two
    for (std::size_t i = 0; i < real.size(); ++i) CHECK_NEAR(real[i], original[i], 1e-12);
}

SVJ_TEST("spectrum: a tone lights the band it falls in and not its neighbours") {
    SpectrumSettings settings;
    settings.band_count = 8;
    SpectrumAnalyser analyser(1024, kRate, settings);

    feed_sine(analyser, 1000.0, 0.5);

    const std::size_t lit = band_containing(analyser, 1000.0);
    CHECK(lit < analyser.band_count());
    const std::vector<float>& bands = analyser.bands();
    CHECK(bands[lit] > 0.5f);
    for (std::size_t i = 0; i < bands.size(); ++i) {
        if (i == lit) continue;
        // The Hann window is what makes this true, and that was measured rather
        // than assumed: replacing it with a rectangular window fails this test
        // and only this one. 1 kHz does not land on a bin at this window size,
        // so without a window the tone leaks across the whole spectrum and
        // every band lights at once -- which reads as "audio-reactive" until
        // you notice it never stops.
        CHECK(bands[i] < bands[lit] * 0.5f);
    }
}

SVJ_TEST("spectrum: bands are spaced logarithmically, so bass gets its own") {
    SpectrumSettings settings;
    settings.band_count = 8;
    settings.low_hz = 40.0;
    settings.high_hz = 16000.0;
    SpectrumAnalyser analyser(2048, kRate, settings);

    // Every band strictly above the last, and each wider than the one below it.
    // Linear spacing would put seven of the eight above 2 kHz and leave a single
    // band for everything a kick drum lives in.
    for (std::size_t i = 0; i + 1 < analyser.band_count(); ++i) {
        CHECK(analyser.band_high_hz(i) <= analyser.band_low_hz(i + 1) + 1e-9);
        CHECK(analyser.band_low_hz(i) < analyser.band_high_hz(i));
    }
    const double lowest = analyser.band_high_hz(0) - analyser.band_low_hz(0);
    const double highest = analyser.band_high_hz(analyser.band_count() - 1) -
                           analyser.band_low_hz(analyser.band_count() - 1);
    CHECK(highest > lowest * 4.0);
}

SVJ_TEST("spectrum: the block size the caller uses does not change the result") {
    // A real audio callback hands over whatever its device chose. If the answer
    // depended on that, the picture would differ between machines for identical
    // sound -- the sort of fault that is blamed on the GPU for a week.
    SpectrumSettings settings;
    settings.band_count = 6;

    SpectrumAnalyser small(1024, kRate, settings);
    SpectrumAnalyser large(1024, kRate, settings);

    const auto total = static_cast<std::size_t>(kRate * 0.4);
    std::vector<float> signal(total);
    for (std::size_t i = 0; i < total; ++i) {
        const double t = static_cast<double>(i) / kRate;
        signal[i] = static_cast<float>(0.6 * std::sin(2.0 * kPi * 220.0 * t) +
                                       0.3 * std::sin(2.0 * kPi * 3500.0 * t));
    }
    for (std::size_t i = 0; i < total; i += 64) {
        small.push(signal.data() + i, std::min<std::size_t>(64, total - i));
    }
    for (std::size_t i = 0; i < total; i += 999) {
        large.push(signal.data() + i, std::min<std::size_t>(999, total - i));
    }

    for (std::size_t b = 0; b < small.band_count(); ++b) {
        CHECK_NEAR(small.bands()[b], large.bands()[b], 1e-6);
    }
}

SVJ_TEST("spectrum: silence reads as zero rather than as a floor") {
    SpectrumSettings settings;
    settings.band_count = 4;
    SpectrumAnalyser analyser(1024, kRate, settings);

    std::vector<float> quiet(4096, 0.0f);
    analyser.push(quiet.data(), quiet.size());
    for (float band : analyser.bands()) CHECK_NEAR(band, 0.0f, 1e-6);
}

SVJ_TEST("spectrum: a band rises fast and falls slow") {
    // The asymmetry is the point: a kick has to land on the frame it happens,
    // and the picture has to not strobe between frames afterwards.
    SpectrumSettings settings;
    settings.band_count = 4;
    settings.attack_s = 0.005;
    settings.release_s = 0.200;
    SpectrumAnalyser analyser(1024, kRate, settings);

    feed_sine(analyser, 300.0, 0.3);
    const std::size_t lit = band_containing(analyser, 300.0);
    CHECK(lit < analyser.band_count());
    const float loud = analyser.bands()[lit];
    CHECK(loud > 0.5f);

    // Twenty milliseconds of silence: past the attack time several times over,
    // but only a tenth of the release, so most of the level survives.
    analyser.advance_silent(0.020);
    const float after = analyser.bands()[lit];
    CHECK(after < loud);
    CHECK(after > loud * 0.8f);

    // A full second is several release times, so it is effectively gone.
    analyser.advance_silent(1.0);
    CHECK(analyser.bands()[lit] < loud * 0.05f);
}

SVJ_TEST("spectrum: a louder tone reads higher than a quiet one") {
    SpectrumSettings settings;
    settings.band_count = 6;

    SpectrumAnalyser loud(1024, kRate, settings);
    SpectrumAnalyser quiet(1024, kRate, settings);
    feed_sine(loud, 800.0, 0.4, 0.9);
    feed_sine(quiet, 800.0, 0.4, 0.05);

    const std::size_t lit = band_containing(loud, 800.0);
    CHECK(lit < loud.band_count());
    CHECK(loud.bands()[lit] > quiet.bands()[lit] + 0.2f);
}

SVJ_TEST("spectrum: an unusable window size falls back instead of misbehaving") {
    SpectrumSettings settings;
    settings.band_count = 4;
    SpectrumAnalyser analyser(1000, kRate, settings);  // not a power of two
    feed_sine(analyser, 500.0, 0.2);
    // It still produces a usable spectrum rather than silently doing nothing.
    bool any = false;
    for (float band : analyser.bands()) any = any || band > 0.01f;
    CHECK(any);
}

SVJ_TEST("spectrum: band edges stay distinct even when bins are scarce") {
    // At a small window the lowest bands are narrower than one bin. Collapsing
    // them onto the same bin would hand the mapping layer several bands that
    // always report the same number, which looks like a broken mapping.
    SpectrumSettings settings;
    settings.band_count = 12;
    settings.low_hz = 40.0;
    SpectrumAnalyser analyser(256, kRate, settings);
    for (std::size_t i = 0; i + 1 < analyser.band_count(); ++i) {
        CHECK(analyser.band_low_hz(i) < analyser.band_high_hz(i) ||
              analyser.band_high_hz(i) >= kRate * 0.5 - 1e-9);
    }
}
