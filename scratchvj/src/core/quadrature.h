// scratchvj — reading a bare quadrature carrier as a hand's motion.
//
// WHY THIS EXISTS, measured rather than assumed. An MWM Phase receiver does not
// put a control signal on its RCA outputs: it puts a clean 1000 Hz quadrature
// carrier and nothing else. Six seconds of it, analysed to the spectrum, give a
// single line with sidebands 1% high and an amplitude envelope flat to 0.2%. A
// real control record dips to about half on every zero bit -- that modulation IS
// the position -- so xwax's timecoder can never lock on this, and nine
// definitions across every channel pair confirmed it never does. Stop the
// platter and the carrier vanishes, so it is genuinely tracking the record.
//
// What the carrier carries is exact: direction, in the sign of the phase
// difference, and speed, in how fast the phase advances. That is the whole of
// the hand's gesture. What is missing is where the needle sits -- which this
// chain never had, the Phase being a relative device that MWM themselves
// prescribe REL mode for.
//
// IS THE TURN COUNT THE INTEGRATOR PRINCIPLE 1 FORBIDS? The rule bans
// `pos += rate * dt`: accumulating an ESTIMATE, whose error is kept forever and
// grows as a random walk. Here the phase is MEASURED every sample, absolutely,
// as the angle of a vector whose two components are both present in the signal.
// The only accumulated quantity is an integer count of complete turns, and an
// integer does not drift -- it is exactly right, or it takes one discrete step.
// So the refinement this module adds to principle 1 is:
//
//     An accumulator is allowed only when its increment is EXACT rather than
//     estimated, and when its failure is a DISCRETE, DETECTABLE event rather
//     than silent drift.
//
// That exemption has to be earned, not asserted, and the code below earns it in
// two places: the slew guard makes an over-Nyquist turn observable instead of
// silently reversing, and the coherence lock makes a lost carrier observable
// instead of freewheeling on noise. Without those two, the argument would be a
// claim. `slew_events()` is how you check it after a set.
//
// Note what is NOT claimed: that this is an absolute position. It is position
// relative to the last reset(), which is exactly what TransportMode::Relative
// and core/anchor already exist to handle.
//
// Zero dependency, like the rest of core/, so all of it is tested without a
// turntable -- and unlike the xwax path, the generator below can produce REVERSE
// motion, which is what finally makes scratching itself testable.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "core/timecode.h"

namespace svj {

struct QuadratureConfig {
    // Nominal carrier at nominal record speed. 1000 Hz for the Serato family,
    // which is what the Phase emits; Traktor is 2000, MixVibes 1300.
    //
    // This is a SCALE CALIBRATION, not a detection parameter: getting it wrong
    // by a factor scales every velocity by that factor and changes nothing else.
    // It should be measured, the way the channel order is, never guessed.
    double carrier_hz = 1000.0;
    double sample_rate = 48000.0;

    // Where position_s starts after a reset(). Zero is honest -- the position is
    // relative and says so -- but a source that wants to look like a record can
    // start mid-side.
    double origin_s = 0.0;

    // Radius below which the carrier is treated as absent. Deliberately low:
    // level alone has already produced a false positive on this project (a
    // webcam hearing a quiet room measured 0.012 and passed a quadrature test),
    // so it is COHERENCE that does the real work, not this floor.
    float silence_level = 0.004f;

    // Largest phase step accepted between two samples, as a fraction of pi.
    // Past pi the unwrap cannot tell forwards from backwards, and the failure is
    // the worst possible: a hard forward throw reads as a backspin. 0.8 leaves
    // room for noise while sitting far above any real gesture -- roughly 19x
    // nominal at 48 kHz, where real scratches live at 4 to 10.
    double slew_limit_pi = 0.8;

    // Window the velocity is fitted over. Short enough to catch a scratch, long
    // enough that one noisy sample is not a direction change.
    double velocity_window_s = 0.010;
};

class QuadratureTracker {
public:
    // Returns false on a configuration that cannot work: a non-positive carrier,
    // or a sample rate at or below twice the carrier, where even a stationary
    // record could not be tracked.
    bool configure(const QuadratureConfig& config);
    const QuadratureConfig& config() const { return config_; }

    // The speed ratio at which the carrier reaches Nyquist and the unwrap starts
    // reversing. Exposed so the app can set TimecodeConfig::max_speed_ratio from
    // it rather than keeping a second, independently-chosen constant: at 48 kHz
    // the two happen to agree at 24x, and at 44.1 kHz they do not.
    double max_speed_ratio() const;

    // Forgets the accumulated position, keeping the configuration. This is what
    // a re-anchor does; the position is relative by nature, so "where we are" is
    // always relative to the last reset.
    void reset();

    // Feeds one block of INTERLEAVED STEREO samples and reports what the tracker
    // now knows. Shaped exactly like svj::dvs::TimecodeDecoder::submit so the
    // two sources are interchangeable at the call site -- DecoderSample IS the
    // interface between them, with no virtual base anywhere.
    //
    // The two channels are the legs of the quadrature pair; their order decides
    // which way is forwards, and that is a wiring fact calibrated against real
    // gear rather than decided here.
    DecoderSample submit(const float* interleaved, std::size_t frames, double now_s);

    double position_s() const { return position_s_; }
    float velocity() const { return velocity_; }
    float level() const { return level_; }
    bool locked() const { return locked_; }

    // How many times the slew guard has fired since reset(). Non-zero means the
    // record was thrown past what this sample rate can track, and the position
    // has lost that much. It is the observable that makes the principle-1
    // argument above a property rather than a promise.
    std::uint32_t slew_events() const { return slew_events_; }

private:
    void update_calibration(double l, double r);

    QuadratureConfig config_;

    // Heydemann correction: a quadrature pair is never a perfect circle. An
    // off-centre or elliptical Lissajous makes atan2 run fast on one side and
    // slow on the other, which is speed ripple at the carrier rate on a moving
    // platter and jitter that reads as micro-scratching at a standstill. Rolling
    // min/max per channel gives the centre and the gain.
    double centre_[2] = {0.0, 0.0};
    double gain_[2] = {1.0, 1.0};
    double range_min_[2] = {0.0, 0.0};
    double range_max_[2] = {0.0, 0.0};
    std::size_t calibration_samples_ = 0;
    double turned_since_calibration_ = 0.0;

    long long turns_ = 0;
    double phase_ = 0.0;
    bool have_phase_ = false;

    double position_s_ = 0.0;
    float velocity_ = 0.0f;
    float level_ = 0.0f;
    double radius_ = 0.0;
    bool locked_ = false;
    std::uint32_t coherent_run_ = 0;
    std::uint32_t slew_events_ = 0;

    // Position history the velocity is fitted over, so velocity is a READING of
    // measured positions and can never disagree with the position itself.
    struct Mark {
        double t = 0.0;
        double position = 0.0;
    };
    std::vector<Mark> history_;
    std::size_t history_head_ = 0;
    std::size_t history_count_ = 0;
    std::size_t since_mark_ = 0;
    double elapsed_s_ = 0.0;
};

// --- the generator, for testing without a turntable -------------------------
//
// Its own generator rather than dvs/generate, and the reason is decisive:
// dvs/generate deliberately cannot produce REVERSE motion, because a control
// record's LFSR runs backwards and pretending otherwise would test against a
// signal no record can make. With no bitstream, this one can -- so a scratch,
// the thing this instrument exists for, becomes testable in the default build
// with no hardware and no GPL-3 dependency.
struct QuadratureSignal {
    double carrier_hz = 1000.0;
    double sample_rate = 48000.0;
    double amplitude = 0.5;
    // Defects worth reproducing, each matching a test below.
    double dc_left = 0.0;
    double dc_right = 0.0;
    double gain_right = 1.0;
    double noise = 0.0;
    std::uint32_t seed = 1;
    bool swap_channels = false;
};

// `position_at(t)` gives the record's position in seconds at time t. It is
// EVALUATED, never integrated -- the test rig obeys the same rule the module
// does, so a drifting generator can never be mistaken for a drifting tracker.
bool generate_quadrature(const QuadratureSignal& signal,
                         const std::function<double(double)>& position_at,
                         std::size_t frames, std::vector<float>& out);

}  // namespace svj
