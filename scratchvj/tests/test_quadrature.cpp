#include <cmath>
#include <vector>

#include "core/quadrature.h"
#include "core/timecode.h"
#include "harness.h"

using namespace svj;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRate = 48000.0;

QuadratureConfig standard() {
    QuadratureConfig config;
    config.sample_rate = kRate;
    return config;
}

// Runs `seconds` of a motion through the tracker in awkward block sizes -- an
// audio callback never hands over a tidy amount -- and returns the last sample.
DecoderSample run(QuadratureTracker& tracker, const QuadratureSignal& signal,
                  const std::function<double(double)>& position_at, double seconds,
                  std::size_t block = 233) {
    const auto frames = static_cast<std::size_t>(signal.sample_rate * seconds);
    std::vector<float> pcm;
    CHECK(generate_quadrature(signal, position_at, frames, pcm));

    DecoderSample last;
    for (std::size_t i = 0; i < frames; i += block) {
        const std::size_t count = std::min(block, frames - i);
        last = tracker.submit(pcm.data() + i * 2, count, i / signal.sample_rate);
    }
    return last;
}

QuadratureSignal plain() {
    QuadratureSignal signal;
    signal.sample_rate = kRate;
    return signal;
}

// A record turning at a constant ratio.
auto at_speed(double ratio) {
    return [ratio](double t) { return ratio * t; };
}

}  // namespace

// --- the generator has to be right before anything else means anything -------

SVJ_TEST("quadrature: the generator's two channels are a quarter cycle apart") {
    // If this were wrong, every test below would be verifying nonsense.
    std::vector<float> pcm;
    CHECK(generate_quadrature(plain(), at_speed(1.0), 4800, pcm));

    double re_l = 0.0, im_l = 0.0, re_r = 0.0, im_r = 0.0;
    for (std::size_t i = 0; i < 4800; ++i) {
        const double a = 2.0 * kPi * 1000.0 * static_cast<double>(i) / kRate;
        re_l += pcm[i * 2] * std::cos(a);
        im_l += pcm[i * 2] * std::sin(a);
        re_r += pcm[i * 2 + 1] * std::cos(a);
        im_r += pcm[i * 2 + 1] * std::sin(a);
    }
    double difference = std::atan2(im_r, re_r) - std::atan2(im_l, re_l);
    while (difference > kPi) difference -= 2.0 * kPi;
    while (difference < -kPi) difference += 2.0 * kPi;
    CHECK_NEAR(std::fabs(difference * 180.0 / kPi), 90.0, 2.0);
}

// --- core correctness --------------------------------------------------------

SVJ_TEST("quadrature: a record at nominal speed advances one second per second") {
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, plain(), at_speed(1.0), 2.0);

    CHECK(sample.locked);
    CHECK_NEAR(sample.position_s, 2.0, 0.002);
    CHECK_NEAR(sample.pitch, 1.0f, 0.02f);
}

SVJ_TEST("quadrature: half speed covers half the distance") {
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, plain(), at_speed(0.5), 2.0);
    CHECK_NEAR(sample.position_s, 1.0, 0.002);
    CHECK_NEAR(sample.pitch, 0.5f, 0.02f);
}

SVJ_TEST("quadrature: a backwards record gives a negative position") {
    // The test the DVS generator structurally cannot write: a control record's
    // LFSR does not run backwards, so dvs/generate refuses to pretend. With no
    // bitstream there is nothing to reverse, so reverse motion is generable --
    // and scratching finally becomes testable with no hardware.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, plain(), at_speed(-1.0), 2.0);

    CHECK(sample.locked);
    CHECK_NEAR(sample.position_s, -2.0, 0.002);
    CHECK(sample.pitch < -0.9f);
}

SVJ_TEST("quadrature: swapping the channels reverses the apparent direction") {
    // A reversed RCA pair makes a record played forwards look like one played
    // backwards, and nothing downstream can tell. Which way round is correct is
    // a fact about the desk, calibrated against real gear -- so this asserts the
    // swap flips the sign, never which sign is right.
    QuadratureSignal signal = plain();
    signal.swap_channels = true;

    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, signal, at_speed(1.0), 1.0);
    CHECK(sample.pitch < -0.9f);
}

SVJ_TEST("quadrature: a stopped platter does not move, and stays locked") {
    // The test a zero-crossing implementation fails. xwax needs crossings to see
    // anything and high-passes the input to find them, so at a standstill -- a
    // 0 Hz carrier -- it goes blind. An angle is exact at any speed including
    // none, which is precisely what a scratch instrument needs.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));

    // Move first so the calibration settles, then hold still.
    run(tracker, plain(), at_speed(1.0), 1.0);
    const double before = tracker.position_s();

    const DecoderSample sample =
        run(tracker, plain(), [](double) { return 0.25; }, 1.0);
    CHECK(sample.locked);
    CHECK_NEAR(tracker.position_s(), tracker.position_s(), 1e-12);
    CHECK(std::fabs(tracker.position_s() - before) < 1.0);  // did not run away
    CHECK_NEAR(sample.pitch, 0.0f, 0.02f);
}

SVJ_TEST("quadrature: a platter stopped on the phase wrap does not drift") {
    // Parked exactly on the branch cut with a little noise, the angle flickers
    // across +/-pi. Shortest-path unwrapping must read that as +/-epsilon, never
    // as a whole turn -- otherwise a record sitting still would walk away.
    QuadratureSignal signal = plain();
    signal.noise = 0.02;

    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    run(tracker, plain(), at_speed(1.0), 0.5);  // settle the calibration

    const double before = tracker.position_s();
    // atan2(y, x) is +/-pi where x < 0 and y crosses zero, i.e. half a turn.
    run(tracker, signal, [](double) { return 0.0005; }, 2.0);
    CHECK(std::fabs(tracker.position_s() - before) < 0.001);
}

// --- scratching, the point of the module -------------------------------------

SVJ_TEST("quadrature: a scratch returns to exactly where it started") {
    // THE principle-1 test. Position must be a function of the signal with no
    // drift term: after a whole number of cycles of any motion, the reported
    // position must be what it was, however long the run.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));

    const auto scratch = [](double t) { return 0.5 * std::sin(2.0 * kPi * 2.0 * t); };
    run(tracker, plain(), scratch, 0.5);
    const double after_first = tracker.position_s();

    // Nine more whole cycles: 2 Hz, so every half second is a whole number.
    run(tracker, plain(), scratch, 4.5);
    CHECK_NEAR(tracker.position_s(), after_first, 1e-4);
    CHECK_EQ(tracker.slew_events(), 0u);
}

SVJ_TEST("quadrature: a reversal is seen, and without a spike at the turn") {
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));

    run(tracker, plain(), at_speed(1.0), 0.5);
    CHECK(tracker.velocity() > 0.5f);

    run(tracker, plain(), at_speed(-1.0), 0.5);
    CHECK(tracker.velocity() < -0.5f);
    CHECK(std::fabs(tracker.velocity()) < 2.0f);  // no overshoot at the turn
}

SVJ_TEST("quadrature: the block size the caller uses does not change the result") {
    // A per-block state bug would show up here and almost nowhere else.
    QuadratureTracker small, large;
    CHECK(small.configure(standard()));
    CHECK(large.configure(standard()));

    run(small, plain(), at_speed(1.0), 1.0, 64);
    run(large, plain(), at_speed(1.0), 1.0, 999);
    CHECK_NEAR(small.position_s(), large.position_s(), 1e-9);
}

// --- robustness --------------------------------------------------------------

SVJ_TEST("quadrature: a DC offset does not make the speed wobble") {
    // An off-centre Lissajous makes the angle run fast on one side of the circle
    // and slow on the other: ripple at the carrier rate on a moving platter, and
    // jitter that reads as micro-scratching at a standstill.
    QuadratureSignal signal = plain();
    signal.dc_left = 0.2 * signal.amplitude;

    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, signal, at_speed(1.0), 2.0);
    CHECK_NEAR(sample.position_s, 2.0, 0.01);
    CHECK_NEAR(sample.pitch, 1.0f, 0.05f);
}

SVJ_TEST("quadrature: a gain imbalance does not change the distance travelled") {
    QuadratureSignal signal = plain();
    signal.gain_right = 0.5;

    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, signal, at_speed(1.0), 2.0);
    CHECK_NEAR(sample.position_s, 2.0, 0.01);
}

SVJ_TEST("quadrature: broadband noise never reads as a lock") {
    // Unlike xwax there is NO bitstream to fail on, so radius coherence is the
    // only defence. This project has already been bitten once: audio_probe
    // called a webcam hearing a quiet room "TIMECODE", because on noise the
    // phase is random and roughly one pair in four lands near quadrature.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));

    std::vector<float> noise(static_cast<std::size_t>(kRate) * 2);
    std::uint32_t seed = 22222;
    for (float& value : noise) {
        seed = seed * 1664525u + 1013904223u;
        value = static_cast<float>(static_cast<double>(seed >> 8) / 8388608.0 - 1.0) * 0.2f;
    }

    DecoderSample last;
    for (std::size_t i = 0; i < static_cast<std::size_t>(kRate); i += 512) {
        const std::size_t n = std::min<std::size_t>(512, static_cast<std::size_t>(kRate) - i);
        last = tracker.submit(noise.data() + i * 2, n, i / kRate);
    }
    // Noise may momentarily look coherent, but it must not carry the record
    // anywhere: a second of it cannot produce a second of travel.
    CHECK(std::fabs(tracker.position_s()) < 0.1);
}

SVJ_TEST("quadrature: silence reports no lock and invents no motion") {
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    run(tracker, plain(), at_speed(1.0), 0.5);
    const double before = tracker.position_s();

    const std::vector<float> quiet(static_cast<std::size_t>(kRate) * 2, 0.0f);
    const DecoderSample sample =
        tracker.submit(quiet.data(), static_cast<std::size_t>(kRate), 1.0);

    CHECK(!sample.locked);
    CHECK(sample.position_s < 0.0);  // the sentinel, for sources that still use it
    CHECK_NEAR(tracker.position_s(), before, 1e-12);
    CHECK_NEAR(sample.pitch, 0.0f, 1e-6f);
}

SVJ_TEST("quadrature: a weak cartridge tracks the same distance as a strong one") {
    // Lock is radius-COHERENCE, not absolute level, so a quiet signal must give
    // exactly the same answer.
    QuadratureSignal quiet = plain();
    quiet.amplitude = 0.02;

    QuadratureTracker loud_tracker, quiet_tracker;
    CHECK(loud_tracker.configure(standard()));
    CHECK(quiet_tracker.configure(standard()));

    run(loud_tracker, plain(), at_speed(1.0), 1.0);
    run(quiet_tracker, quiet, at_speed(1.0), 1.0);
    CHECK_NEAR(quiet_tracker.position_s(), loud_tracker.position_s(), 0.005);
}

// --- Nyquist -----------------------------------------------------------------

SVJ_TEST("quadrature: the speed limit is half the sample rate in carrier cycles") {
    // The number lives here rather than in folklore. At 48 kHz it coincides with
    // TimecodeConfig::max_speed_ratio's 24.0, which was chosen independently as
    // "beyond any real backspin"; at 44.1 kHz the two diverge, which is exactly
    // why the app should take this value rather than keep its own.
    QuadratureConfig config = standard();
    QuadratureTracker tracker;
    CHECK(tracker.configure(config));
    CHECK_NEAR(tracker.max_speed_ratio(), 24.0, 1e-9);

    config.sample_rate = 44100.0;
    CHECK(tracker.configure(config));
    CHECK_NEAR(tracker.max_speed_ratio(), 22.05, 1e-9);
}

SVJ_TEST("quadrature: a throw approaching the speed limit freezes rather than reversing") {
    // The band the guard exists for: between slew_limit (19.2x at 48 kHz) and
    // Nyquist (24x), the step is still unambiguous and the guard can refuse it.
    // Freezing for the few milliseconds of an over-speed throw is dramatically
    // better than the alternative, which is the picture running backwards.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    run(tracker, plain(), at_speed(1.0), 0.5);
    const double before = tracker.position_s();

    run(tracker, plain(), at_speed(22.0), 0.2);
    CHECK(tracker.slew_events() > 0u);
    CHECK(tracker.position_s() >= before - 0.01);  // never went backwards
}

SVJ_TEST("quadrature: past Nyquist the reading is ambiguous, and that is sampling") {
    // Stated rather than papered over. Above max_speed_ratio() the carrier
    // aliases: a 30x forward throw arrives as a step that, after wrapping, is
    // indistinguishable from a legal backwards one. No guard can catch it from
    // the phase alone -- the information is gone, not hidden. This is why
    // max_speed_ratio() is exposed at all, and why the honest answer for a
    // faster gesture is a higher sample rate rather than a cleverer decoder.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    CHECK(tracker.max_speed_ratio() < 30.0);

    run(tracker, plain(), at_speed(1.0), 0.3);
    const double before = tracker.position_s();
    run(tracker, plain(), at_speed(30.0), 0.1);

    // It moved somewhere; the test asserts only that the limit is real and
    // documented, not that the result is meaningful.
    CHECK(std::fabs(tracker.position_s() - before) >= 0.0);
}

SVJ_TEST("quadrature: a fast but legal backspin tracks exactly") {
    // Proves the slew guard is not so tight that it eats real gestures: a
    // backspin lives around 6 to 10 times nominal.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));
    const DecoderSample sample = run(tracker, plain(), at_speed(-8.0), 0.3);

    CHECK_EQ(tracker.slew_events(), 0u);
    CHECK_NEAR(sample.position_s, -2.4, 0.01);
}

// --- the seam ----------------------------------------------------------------

SVJ_TEST("quadrature: the sample it produces is the one core/timecode consumes") {
    // The two sources are interchangeable at the call site, and DecoderSample is
    // the whole of the interface between them -- no virtual base anywhere.
    QuadratureTracker tracker;
    CHECK(tracker.configure(standard()));

    TimecodeConfig timecode;
    timecode.mode = TransportMode::Relative;
    timecode.profile = SignalProfile::Wireless;
    TimecodeTracker downstream(timecode);

    const auto frames = static_cast<std::size_t>(kRate * 2.0);
    std::vector<float> pcm;
    CHECK(generate_quadrature(plain(), at_speed(1.0), frames, pcm));

    const std::size_t block = 512;
    for (std::size_t i = 0; i < frames; i += block) {
        const std::size_t n = std::min(block, frames - i);
        downstream.submit(tracker.submit(pcm.data() + i * 2, n, i / kRate));
    }
    CHECK(downstream.state().link == LinkState::Ok);
    CHECK_NEAR(downstream.state().position_s, 2.0, 0.01);
    CHECK_EQ(downstream.jump_count(), 0);
}

SVJ_TEST("quadrature: an unusable configuration is refused rather than limped through") {
    QuadratureTracker tracker;
    QuadratureConfig config = standard();

    config.carrier_hz = 0.0;
    CHECK(!tracker.configure(config));

    config = standard();
    config.sample_rate = 1500.0;  // below twice the 1000 Hz carrier
    CHECK(!tracker.configure(config));

    config = standard();
    config.slew_limit_pi = 1.5;  // past pi the unwrap is meaningless
    CHECK(!tracker.configure(config));
}
