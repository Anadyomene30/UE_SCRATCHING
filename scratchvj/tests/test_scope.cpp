#include <algorithm>
#include <cmath>
#include <vector>

#include "core/quadrature.h"
#include "core/scope.h"
#include "harness.h"

using namespace svj;

namespace {

constexpr double kRate = 48000.0;

QuadratureSignal plain() {
    QuadratureSignal signal;
    signal.sample_rate = kRate;
    return signal;
}

auto at_speed(double ratio) {
    return [ratio](double t) { return ratio * t; };
}

// Runs `seconds` of a signal through the scope in awkward block sizes -- an
// audio callback never hands over a tidy amount, and the window boundary must
// not care where the blocks fall.
void run(LissajousScope& scope, const QuadratureSignal& signal, double ratio,
         double seconds, std::size_t block = 233) {
    const auto frames = static_cast<std::size_t>(signal.sample_rate * seconds);
    std::vector<float> pcm;
    CHECK(generate_quadrature(signal, at_speed(ratio), frames, pcm));
    for (std::size_t i = 0; i < frames; i += block) {
        scope.submit(pcm.data() + i * 2, std::min(block, frames - i));
    }
}

}  // namespace

SVJ_TEST("scope: a clean carrier at nominal speed reads as a centred circle") {
    LissajousScope scope;
    run(scope, plain(), 1.0, 0.5);

    const ScopeReading& r = scope.reading();
    CHECK(r.verdict == ScopeVerdict::Measured);
    CHECK_NEAR(r.centre_x, 0.0, 0.005);
    CHECK_NEAR(r.centre_y, 0.0, 0.005);
    CHECK_NEAR(r.amplitude_x, 0.5, 0.01);
    CHECK_NEAR(r.amplitude_y, 0.5, 0.01);
    CHECK_NEAR(r.balance_db, 0.0, 0.1);
    CHECK_NEAR(r.phase_error_deg, 0.0, 0.5);
    // 100 ms of a 1000 Hz carrier.
    CHECK_NEAR(r.turns, 100.0, 1.0);
}

SVJ_TEST("scope: a DC offset is reported as a centre, never as an amplitude") {
    // The trap this catches: measuring amplitude as an RMS about ZERO rather
    // than about the measured centre. That reads an offset leg as a louder
    // one, and sends the operator to the trim knob for a fault the trim knob
    // cannot fix.
    QuadratureSignal signal = plain();
    signal.dc_left = 0.1;

    LissajousScope scope;
    run(scope, signal, 1.0, 0.5);

    const ScopeReading& r = scope.reading();
    CHECK(r.verdict == ScopeVerdict::Measured);
    CHECK_NEAR(r.centre_x, 0.1, 0.005);
    CHECK_NEAR(r.centre_y, 0.0, 0.005);
    CHECK_NEAR(r.amplitude_x, 0.5, 0.01);
    CHECK_NEAR(r.balance_db, 0.0, 0.15);
}

SVJ_TEST("scope: one louder leg reads as balance in dB and leaves the phase alone") {
    QuadratureSignal signal = plain();
    signal.gain_right = 2.0;

    LissajousScope scope;
    run(scope, signal, 1.0, 0.5);

    const ScopeReading& r = scope.reading();
    CHECK(r.verdict == ScopeVerdict::Measured);
    CHECK_NEAR(r.balance_db, 6.02, 0.1);
    CHECK_NEAR(r.phase_error_deg, 0.0, 0.5);
}

SVJ_TEST("scope: crosstalk reads as a phase error and leaves the balance alone") {
    // Gain and phase both turn the circle into an ellipse, and this pair of
    // tests is the whole justification for reporting them separately: an
    // operator who cannot tell them apart turns the wrong knob. A gain fault
    // must not show up as phase, and a phase fault must not show up as gain.
    QuadratureSignal signal = plain();
    signal.phase_deg = 20.0;

    LissajousScope scope;
    run(scope, signal, 1.0, 0.5);

    const ScopeReading& r = scope.reading();
    CHECK(r.verdict == ScopeVerdict::Measured);
    CHECK_NEAR(r.phase_error_deg, 20.0, 0.5);
    CHECK_NEAR(r.balance_db, 0.0, 0.1);
}

SVJ_TEST("scope: a platter turning too slowly is refused, not fitted to an arc") {
    // On this chain the carrier frequency IS the platter speed, so at a crawl
    // the window holds an arc rather than a loop. An ellipse fitted to an arc
    // is a guess, and a guess printed in degrees next to a real measurement is
    // indistinguishable from one.
    LissajousScope scope;
    run(scope, plain(), 0.02, 0.5);  // 20 Hz: two turns per 100 ms window

    const ScopeReading& r = scope.reading();
    CHECK(r.verdict == ScopeVerdict::TooSlow);
    CHECK(r.windows > 0);
    CHECK_NEAR(r.turns, 2.0, 1.0);
}

SVJ_TEST("scope: silence is no signal rather than a perfectly centred figure") {
    LissajousScope scope;
    std::vector<float> quiet(9600 * 2, 0.0f);
    scope.submit(quiet.data(), 9600);

    CHECK(scope.reading().verdict == ScopeVerdict::NoSignal);
    CHECK(scope.reading().windows > 0);
}

SVJ_TEST("scope: nothing is published before a window has closed") {
    LissajousScope scope;
    run(scope, plain(), 1.0, 0.05);  // half a window

    CHECK_EQ(scope.reading().windows, 0u);
    CHECK(scope.reading().verdict == ScopeVerdict::NoSignal);
}

SVJ_TEST("scope: the trace is a continuous walk, not a stroboscope") {
    // The trap: drawing the figure from every Nth sample. A stride that
    // divides the carrier period lands every point at the same phase and
    // draws a dot -- or a triangle, which looks like a real and alarming
    // fault -- out of a perfectly good circle. Consecutive points of an
    // untouched trace are neighbours on the circle; decimated ones are not.
    LissajousScope scope;
    run(scope, plain(), 1.0, 0.5);

    std::vector<float> points;
    scope.trace(points);
    CHECK_EQ(points.size(), scope.config().trace * 2);

    const double amplitude = static_cast<double>(scope.reading().amplitude_x);
    double longest_step = 0.0;
    bool quadrant[4] = {false, false, false, false};
    for (std::size_t i = 0; i < points.size() / 2; ++i) {
        const double x = points[i * 2];
        const double y = points[i * 2 + 1];
        quadrant[(x >= 0.0 ? 0u : 1u) + (y >= 0.0 ? 0u : 2u)] = true;
        if (i > 0) {
            const double dx = x - points[(i - 1) * 2];
            const double dy = y - points[(i - 1) * 2 + 1];
            longest_step = std::max(longest_step, std::sqrt(dx * dx + dy * dy));
        }
    }
    for (bool visited : quadrant) CHECK(visited);
    CHECK(longest_step < amplitude * 0.5);
}

SVJ_TEST("scope: a reconfigure clears the figure instead of mixing two windows") {
    LissajousScope scope;
    run(scope, plain(), 1.0, 0.5);
    CHECK(scope.reading().windows > 0);

    ScopeConfig config;
    config.trace = 128;
    CHECK(scope.configure(config));
    CHECK_EQ(scope.reading().windows, 0u);

    std::vector<float> points;
    scope.trace(points);
    CHECK(points.empty());

    // And a configuration that could never produce a reading is refused.
    ScopeConfig empty;
    empty.window = 0;
    CHECK(!scope.configure(empty));
}
