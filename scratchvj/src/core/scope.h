// scratchvj — the timecode carrier as a figure, for calibrating by eye.
//
// WHY A FIGURE AND NOT A NUMBER. `core/quadrature` already reports level,
// lock and velocity, and all three can look perfectly healthy while the chain
// is badly adjusted: a cartridge whose two channels differ by 3 dB still locks,
// still tracks, and still turns every scratch into speed ripple at the carrier
// rate. What is wrong is the SHAPE of the pair, and the shape has been the
// standard instrument for this since the oscilloscope -- x against y, a circle
// when the two legs are equal and square, an ellipse when they are not.
//
// WHAT THIS MEASURES, AND WHY THESE THREE. A quadrature pair has exactly three
// ways to stop being a centred circle, and each one is a different knob on a
// different piece of hardware:
//
//   - a CENTRE away from the origin is a DC offset -- a coupling capacitor, or
//     an input stage sitting off zero;
//   - a BALANCE away from 0 dB is one leg louder than the other -- a channel
//     trim, a tired stylus tip, an unbalanced cartridge;
//   - a PHASE ERROR away from a right angle is crosstalk between the legs, and
//     it tilts the ellipse at 45 degrees instead of squashing it along an axis.
//
// Reporting them separately is the whole point: gain and phase both turn the
// circle into an ellipse, and an operator who cannot tell them apart will turn
// the wrong knob. `phase_error_deg` is what distinguishes them.
//
// WHY NOT REUSE THE TRACKER'S NUMBERS. `QuadratureTracker` keeps a centre and a
// gain too, by rolling min/max, and CORRECTS by them. Two reasons this module
// measures its own: what is being adjusted is the signal BEFORE correction, so
// showing the corrected figure would show a circle no matter how bad the input
// is; and min/max is two samples, which one click ruins, where an RMS over a
// window is what a needle drop cannot poison.
//
// WHAT IT CANNOT SHOW. Which way round the figure is traced -- that is the
// channel order, a different question, and a still picture never answers it.
// The tracker's velocity sign answers it, and `QuadratureSignal::swap_channels`
// is how it was calibrated against real gear.
//
// Zero dependency like the rest of core/, so the whole thing is tested against
// `generate_quadrature` with no turntable in the room.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace svj {

struct ScopeConfig {
    // Samples averaged into one reading. 4800 is 100 ms at 48 kHz: a hundred
    // turns of a nominal carrier, so the mean that gives the centre lands
    // within a few parts per thousand of the true offset, and slow enough that
    // the numbers on screen can be read rather than watched flickering.
    std::size_t window = 4800;

    // Samples kept for drawing. NOT a decimation of the window, deliberately:
    // taking every Nth sample of a periodic signal is a stroboscope, and a
    // stride that happens to divide the carrier period would draw a triangle,
    // or a single dot, out of a perfectly good circle -- a display that lies
    // exactly when it is most trusted. Keeping the most recent samples
    // untouched costs a few kilobytes and cannot alias.
    std::size_t trace = 512;

    // Below this amplitude on both legs there is nothing to measure. Same
    // floor as QuadratureConfig::silence_level, and for the same reason: on
    // this chain a stopped platter emits no carrier at all.
    float silence_level = 0.004f;

    // Whole carrier turns the window must contain before the figure means
    // anything. Under this the pair is an arc, not a loop, and an ellipse
    // fitted to an arc is a guess. Four is the smallest count at which all
    // four quadrants are certainly visited.
    double min_turns = 4.0;
};

enum class ScopeVerdict {
    // Nothing on either leg: no device, or a platter at rest.
    NoSignal,
    // A carrier, but too little of it turned in the window to fit a shape.
    // The honest reading at a standstill and during a slow hand-hold.
    TooSlow,
    // The three numbers below are a measurement.
    Measured,
};

struct ScopeReading {
    ScopeVerdict verdict = ScopeVerdict::NoSignal;

    // The centre of the figure in signal units, i.e. the DC offset per leg.
    float centre_x = 0.0f;
    float centre_y = 0.0f;

    // Peak amplitude per leg, from the RMS about that centre.
    float amplitude_x = 0.0f;
    float amplitude_y = 0.0f;

    // 20*log10(y/x): zero when the two legs match, positive when the right
    // one is the louder. Decibels because that is the unit written on the
    // trim it is adjusted with.
    float balance_db = 0.0f;

    // Departure from a right angle, in degrees, positive when the right leg
    // leads by more than 90. Zero on a perfect pair.
    float phase_error_deg = 0.0f;

    // Whole carrier turns the window held. Says how fast the platter was
    // going, and is what TooSlow is decided on.
    float turns = 0.0f;

    // Readings published since reset(), so a caller can tell a stale figure
    // from a fresh one without comparing floats.
    std::uint32_t windows = 0;
};

class LissajousScope {
public:
    LissajousScope();

    // Returns false on a configuration that could not produce a reading: an
    // empty window or an empty trace.
    bool configure(const ScopeConfig& config);
    const ScopeConfig& config() const { return config_; }

    // Forgets the accumulating window, the published reading and the trace.
    void reset();

    // Feeds one block of INTERLEAVED STEREO samples -- the same block
    // core/quadrature is given, from the same drain, so the figure and the
    // tracking can never be looking at different audio.
    void submit(const float* interleaved, std::size_t frames);

    const ScopeReading& reading() const { return reading_; }

    // The figure, oldest point first, interleaved x,y. Shorter than
    // `config().trace` until that many samples have arrived; `out` is
    // replaced, not appended to.
    void trace(std::vector<float>& out) const;

private:
    void publish();

    ScopeConfig config_;
    ScopeReading reading_;

    // Sums over the window in progress. Doubles: the squares of a hundred
    // thousand samples are exactly where a float accumulator starts losing
    // the small differences this module exists to see.
    double sum_x_ = 0.0;
    double sum_y_ = 0.0;
    double sum_xx_ = 0.0;
    double sum_yy_ = 0.0;
    double sum_xy_ = 0.0;
    std::size_t count_ = 0;

    // Turns are counted as zero crossings of the left leg, against the centre
    // the PREVIOUS window measured -- the current one is not known until the
    // window closes. When the offset grows past the amplitude the crossings
    // stop, the count falls to zero and the verdict becomes TooSlow, which is
    // the right answer for a leg that has left its own range.
    double crossing_level_ = 0.0;
    bool last_above_ = false;
    bool have_last_ = false;
    std::uint32_t crossings_ = 0;

    // The trace, as a ring so a block never has to be shifted.
    std::vector<float> ring_;
    std::size_t ring_head_ = 0;
    std::size_t ring_count_ = 0;
};

}  // namespace svj
