#include "core/spectrum.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;

void fft_core(std::vector<double>& real, std::vector<double>& imaginary, bool inverse) {
    const std::size_t n = real.size();
    if (n < 2 || (n & (n - 1)) != 0 || imaginary.size() != n) return;

    // Bit reversal: the decimation-in-time butterflies below expect the input in
    // reversed-index order, and permuting first is cheaper than indexing through
    // a reversal on every pass.
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imaginary[i], imaginary[j]);
        }
    }

    for (std::size_t length = 2; length <= n; length <<= 1) {
        const double angle = (inverse ? 2.0 : -2.0) * kPi / static_cast<double>(length);
        const double wr = std::cos(angle);
        const double wi = std::sin(angle);
        for (std::size_t start = 0; start < n; start += length) {
            double cr = 1.0;
            double ci = 0.0;
            for (std::size_t k = 0; k < length / 2; ++k) {
                const std::size_t a = start + k;
                const std::size_t b = a + length / 2;
                const double xr = real[b] * cr - imaginary[b] * ci;
                const double xi = real[b] * ci + imaginary[b] * cr;
                real[b] = real[a] - xr;
                imaginary[b] = imaginary[a] - xi;
                real[a] += xr;
                imaginary[a] += xi;
                // The twiddle advanced by repeated multiplication. At the window
                // sizes this is used with (1024, 2048) the drift is far below a
                // float's precision; recomputing cos/sin per butterfly would
                // cost more than the whole rest of the transform.
                const double next = cr * wr - ci * wi;
                ci = cr * wi + ci * wr;
                cr = next;
            }
        }
    }

    if (inverse) {
        const double scale = 1.0 / static_cast<double>(n);
        for (std::size_t i = 0; i < n; ++i) {
            real[i] *= scale;
            imaginary[i] *= scale;
        }
    }
}

}  // namespace

void fft_forward(std::vector<double>& real, std::vector<double>& imaginary) {
    fft_core(real, imaginary, false);
}

void fft_inverse(std::vector<double>& real, std::vector<double>& imaginary) {
    fft_core(real, imaginary, true);
}

SpectrumAnalyser::SpectrumAnalyser(std::size_t window_size, double sample_rate,
                                   const SpectrumSettings& settings)
    : window_size_(window_size),
      sample_rate_(sample_rate > 1.0 ? sample_rate : 48000.0),
      settings_(settings) {
    if (window_size_ < 2 || (window_size_ & (window_size_ - 1)) != 0) window_size_ = 1024;
    if (settings_.band_count < 1) settings_.band_count = 1;

    window_.resize(window_size_);
    for (std::size_t i = 0; i < window_size_; ++i) {
        // Hann. Without a window, a tone that does not land exactly on a bin
        // leaks across the whole spectrum, and every band lights up at once --
        // which looks like "audio-reactive" until you notice it never stops.
        window_[i] = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) /
                                          static_cast<double>(window_size_ - 1));
    }

    ring_.assign(window_size_, 0.0);
    real_.assign(window_size_, 0.0);
    imaginary_.assign(window_size_, 0.0);
    bands_.assign(settings_.band_count, 0.0f);

    // Log-spaced edges, converted to bin indices once. Clamped so that at small
    // window sizes -- where the low bands are narrower than one bin -- the edges
    // stay strictly increasing instead of collapsing several bands onto the same
    // bin and reporting the same number several times.
    const double bin_hz = sample_rate_ / static_cast<double>(window_size_);
    const double low = std::max(settings_.low_hz, bin_hz);
    const double high = std::max(settings_.high_hz, low * 2.0);
    const double ratio = std::log(high / low) / static_cast<double>(settings_.band_count);

    edges_.assign(settings_.band_count + 1, 0);
    const std::size_t last_bin = window_size_ / 2;
    for (std::size_t i = 0; i <= settings_.band_count; ++i) {
        const double hz = low * std::exp(ratio * static_cast<double>(i));
        auto bin = static_cast<std::size_t>(hz / bin_hz + 0.5);
        if (i > 0 && bin <= edges_[i - 1]) bin = edges_[i - 1] + 1;
        edges_[i] = std::min(bin, last_bin);
        if (i > 0 && edges_[i] <= edges_[i - 1]) {
            edges_[i] = std::min(edges_[i - 1] + 1, last_bin);
        }
    }
}

std::size_t SpectrumAnalyser::push(const float* samples, std::size_t count) {
    if (samples == nullptr) return 0;
    std::size_t analysed = 0;
    for (std::size_t i = 0; i < count; ++i) {
        ring_[written_++] = static_cast<double>(samples[i]);
        if (written_ == window_size_) {
            analyse();
            ++analysed;
            // Hop by half a window, so a transient landing near a window edge is
            // still seen near the middle of the next one. Overlapping is what
            // keeps a kick from being attenuated by the window that was supposed
            // to stop it leaking.
            const std::size_t hop = window_size_ / 2;
            std::copy(ring_.begin() + static_cast<std::ptrdiff_t>(hop), ring_.end(),
                      ring_.begin());
            written_ = hop;
        }
    }
    return analysed;
}

void SpectrumAnalyser::analyse() {
    for (std::size_t i = 0; i < window_size_; ++i) {
        real_[i] = ring_[i] * window_[i];
        imaginary_[i] = 0.0;
    }
    fft_forward(real_, imaginary_);

    // Seconds per analysis, for the follower: the hop, not the window, because
    // that is how often this runs.
    const double dt = static_cast<double>(window_size_ / 2) / sample_rate_;
    const auto coefficient = [dt](double tau) {
        if (tau <= 1e-6) return 1.0;
        return 1.0 - std::exp(-dt / tau);
    };
    const double attack = coefficient(settings_.attack_s);
    const double release = coefficient(settings_.release_s);

    const double span = std::max(settings_.ceiling_db - settings_.floor_db, 1e-6);
    // The window halves the amplitude of a full-scale tone, and only half the
    // spectrum is used; normalising here means a full-scale sine reads near the
    // ceiling rather than at some arbitrary number that depends on window size.
    const double normalise = 4.0 / static_cast<double>(window_size_);

    for (std::size_t b = 0; b < bands_.size(); ++b) {
        double peak = 0.0;
        for (std::size_t bin = edges_[b]; bin < edges_[b + 1]; ++bin) {
            const double magnitude =
                std::sqrt(real_[bin] * real_[bin] + imaginary_[bin] * imaginary_[bin]);
            peak = std::max(peak, magnitude * normalise);
        }

        // Peak rather than sum across the band: a sum makes a wide band read
        // louder than a narrow one for the same music, so the top bands would
        // dominate purely by being wider in bins.
        const double db = 20.0 * std::log10(std::max(peak, 1e-9));
        const double target =
            std::clamp((db - settings_.floor_db) / span, 0.0, 1.0);

        const double previous = static_cast<double>(bands_[b]);
        const double rate = target > previous ? attack : release;
        bands_[b] = static_cast<float>(previous + (target - previous) * rate);
    }
}

double SpectrumAnalyser::band_low_hz(std::size_t index) const {
    if (index >= bands_.size()) return 0.0;
    return static_cast<double>(edges_[index]) * sample_rate_ /
           static_cast<double>(window_size_);
}

double SpectrumAnalyser::band_high_hz(std::size_t index) const {
    if (index >= bands_.size()) return 0.0;
    return static_cast<double>(edges_[index + 1]) * sample_rate_ /
           static_cast<double>(window_size_);
}

void SpectrumAnalyser::advance_silent(double seconds) {
    if (seconds <= 0.0) return;
    const double tau = std::max(settings_.release_s, 1e-6);
    const double decay = std::exp(-seconds / tau);
    for (float& band : bands_) band = static_cast<float>(band * decay);
}

void SpectrumAnalyser::reset() {
    std::fill(ring_.begin(), ring_.end(), 0.0);
    written_ = 0;
    std::fill(bands_.begin(), bands_.end(), 0.0f);
}

}  // namespace svj
