// scratchvj — the audio spectrum, as bands the mapping layer can route.
//
// `core/mapping` has had SourceKind::AudioBand and a `bands` pointer since the
// first commit, with nothing to fill it. This is what fills it: a window, an
// FFT, and a split into musically spaced bands.
//
// WHY AN ENVELOPE IS ALLOWED HERE, when design principle 1 forbids integrators.
// The rule is that anything SCRATCHABLE must be a function of position. An audio
// band is not scratchable and cannot be: it is a measurement of sound that
// already happened, and there is no position to evaluate it at. Rewinding the
// record rewinds the AUDIO, and the analyser then measures the rewound audio --
// so the band still follows the hand, through the signal rather than through a
// formula. That is the honest relationship, and the reason the smoothing below
// is a genuine attack/release follower rather than an apology for one.
//
// The distinction that does matter: a take replayed from `core/take` replays the
// recorded band VALUES, not a re-analysis, because the audio is not in the take.
// A band is an input to the instrument, on the same footing as a knob.
//
// No dependency, by the rule that governs all of `core/`: the FFT is forty lines
// of Cooley-Tukey and pulling in a library to avoid writing them would cost more
// than it saves.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace svj {

// In-place radix-2 FFT over interleaved real/imaginary pairs. `size` must be a
// power of two. Exposed because it is worth testing on its own -- a spectrum
// that is subtly wrong looks like "the video is not very reactive" rather than
// like a bug.
void fft_forward(std::vector<double>& real, std::vector<double>& imaginary);
void fft_inverse(std::vector<double>& real, std::vector<double>& imaginary);

// How the bands are laid out and how fast they move.
struct SpectrumSettings {
    std::size_t band_count = 8;
    double low_hz = 40.0;        // below this is rumble and DC offset
    double high_hz = 16000.0;    // above this is air and hiss
    // Attack and release in seconds. Attack fast so a kick lands on the frame it
    // happens; release slower so the picture does not strobe between frames.
    double attack_s = 0.005;
    double release_s = 0.120;
    // Magnitudes are converted to decibels and mapped across this range before
    // being handed out as 0..1. Linear magnitude spends almost its whole range
    // on the loudest transient and leaves everything else near zero, which reads
    // as "nothing is happening" on all but the most compressed material.
    double floor_db = -60.0;
    double ceiling_db = -6.0;
};

// Bands are spaced LOGARITHMICALLY because pitch is. Eight linear bands over
// 40 Hz..16 kHz would put seven of them above the top of a piano, so the bass --
// the part a VJ actually wants to react to -- would share a single band with
// everything below 2 kHz.
class SpectrumAnalyser {
public:
    // `window_size` must be a power of two. It sets the frequency resolution and
    // the latency together: 1024 samples at 48 kHz is 21 ms of both, which is
    // inside the 30 ms the roadmap's latency target allows for the whole chain.
    SpectrumAnalyser(std::size_t window_size, double sample_rate,
                     const SpectrumSettings& settings);

    // Feeds one block of mono samples. Blocks may be any length; the analyser
    // keeps its own ring and analyses whenever it has a full window, so the
    // caller's buffer size never changes the result. Returns how many windows
    // were analysed, which is 0 on a short block and not an error.
    std::size_t push(const float* samples, std::size_t count);

    // The bands, 0..1, in `core/mapping`'s `bands` layout.
    const std::vector<float>& bands() const { return bands_; }
    std::size_t band_count() const { return bands_.size(); }

    // The lower and upper edge of a band, in hertz. For the interface: a band
    // whose range is not visible is a number nobody can map on purpose.
    double band_low_hz(std::size_t index) const;
    double band_high_hz(std::size_t index) const;

    // Silence decays the bands rather than freezing them. Called when audio
    // stops arriving at all -- a disconnected input must not leave the video
    // stuck on whatever the last frame of sound happened to be.
    void advance_silent(double seconds);

    void reset();

private:
    void analyse();

    std::size_t window_size_ = 0;
    double sample_rate_ = 48000.0;
    SpectrumSettings settings_;

    std::vector<double> window_;      // the Hann window, precomputed
    std::vector<double> ring_;        // incoming samples, window_size_ long
    std::size_t written_ = 0;         // how many samples the ring holds
    std::vector<double> real_;
    std::vector<double> imaginary_;
    std::vector<float> bands_;
    std::vector<std::size_t> edges_;  // band_count_ + 1 bin indices
};

}  // namespace svj
