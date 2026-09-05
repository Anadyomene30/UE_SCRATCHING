#include "app/simulation.h"

#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Position of deck A on the control record, and its speed, as a function of the
// script time. Written as a closed form on purpose: the whole engine is built on
// position being a function rather than an accumulation, and the simulation
// obeying the same rule keeps it reproducible from any starting point.
// Every oscillating phase runs a WHOLE number of cycles, so its sine starts and
// ends at zero and the position joins the next phase continuously. A real hand
// cannot teleport, and a script that does trips the discontinuity detector --
// correctly, which is how this was found.
void deck_a_motion(double t, double& position, double& pitch, double& level,
                   std::string& phase) {
    level = 1.0;

    if (t < 4.0) {
        phase = "lecture nominale";
        position = 10.0 + t;
        pitch = 1.0;
    } else if (t < 6.0) {
        phase = "baby scratch";
        const double u = t - 4.0;
        position = 14.0 + 0.35 * std::sin(u * 2.0 * kPi * 2.0);
        pitch = 0.35 * 2.0 * kPi * 2.0 * std::cos(u * 2.0 * kPi * 2.0);
    } else if (t < 7.5) {
        phase = "transform au crossfader";
        const double u = t - 6.0;
        position = 14.0 + u * 1.0;
        pitch = 1.0;
    } else if (t < 9.0) {
        phase = "backspin";
        const double u = t - 7.5;
        position = 15.5 - u * 6.0;
        pitch = -6.0;
    } else if (t < 12.0) {
        phase = "boucle 4 temps, scratch dedans";
        const double u = t - 9.0;
        position = 6.5 + u * 1.0 + 0.25 * std::sin(u * 2.0 * kPi * (5.0 / 3.0));
        pitch = 1.0 + 0.25 * 2.0 * kPi * (5.0 / 3.0) * std::cos(u * 2.0 * kPi * (5.0 / 3.0));
    } else if (t < 13.5) {
        // The moment worth watching: the Phase link drops. The dock stops
        // emitting, which on a control record would mean a stopped platter.
        phase = "LIAISON PERDUE";
        position = 9.5;
        pitch = 0.0;
        level = 0.0;
    } else if (t < 16.0) {
        phase = "re-verrouillage, saut sur hot cue";
        position = 9.5 + (t - 13.5);
        pitch = 1.0;
    } else if (t < 20.0) {
        phase = "scratch rapide, balayage du filtre";
        const double u = t - 16.0;
        position = 12.0 + u * 0.6 + 0.6 * std::sin(u * 2.0 * kPi * 3.0);
        pitch = 0.6 + 0.6 * 2.0 * kPi * 3.0 * std::cos(u * 2.0 * kPi * 3.0);
    } else {
        phase = "retour au calme";
        position = 14.4 + (t - 20.0);
        pitch = 1.0;
    }
}

double triangle(double t, double period) {
    const double u = std::fmod(t, period) / period;
    return u < 0.5 ? u * 2.0 : 2.0 - u * 2.0;
}

}  // namespace

void Simulation::configure(Surface& surface) {
    xfader_ = surface.declare("xfader", ControlKind::Fader);
    hi_ = surface.declare("ch1.eq.hi", ControlKind::Knob);
    mid_ = surface.declare("ch1.eq.mid", ControlKind::Knob);
    filter_ = surface.declare("ch1.filter", ControlKind::Knob);
    fader1_ = surface.declare("ch1.fader", ControlKind::Fader);
    fader2_ = surface.declare("ch2.fader", ControlKind::Fader);
    // ch2's knobs are declared but never moved, so the dashboard shows what an
    // untouched absolute potentiometer looks like: unknown, not zero.
    surface.declare("ch2.eq.hi", ControlKind::Knob);
    surface.declare("ch2.filter", ControlKind::Knob);
}

void Simulation::step(double t_s, Surface& surface, std::uint64_t now_us) {
    const double t = std::fmod(t_s, length_s());
    events_ = SimEvent{};

    double position = 0.0, pitch = 0.0, level = 1.0;
    deck_a_motion(t, position, pitch, level, phase_);

    deck_a_.time_s = t_s;
    deck_a_.position_s = level > 0.0 ? position : -1.0;
    deck_a_.pitch = static_cast<float>(pitch);
    deck_a_.signal_level = static_cast<float>(level);

    deck_b_.time_s = t_s;
    deck_b_.position_s = std::fmod(t_s, 12.0);
    deck_b_.pitch = 1.0f;
    deck_b_.signal_level = 1.0f;

    // Edge-triggered script events: fire once as the script crosses each moment.
    const auto crossed = [this, t](double at) {
        return previous_t_ >= 0.0 && previous_t_ < at && t >= at && t - previous_t_ < 1.0;
    };
    if (crossed(9.0)) { events_.loop_in = true; events_.slip_on = true; }
    if (crossed(12.0)) { events_.loop_exit = true; events_.slip_off = true; }
    if (crossed(14.0)) { events_.cue_jump = true; events_.cue_index = 1; }
    previous_t_ = t;

    // The surface. A transform is a burst of crossfader cuts; the filter sweeps
    // during the fast scratch, which is what the paired effect rack would follow.
    const bool transforming = t >= 6.0 && t < 7.5;
    const double xfader = transforming ? (std::fmod(t * 12.0, 1.0) < 0.5 ? 0.02 : 0.98)
                                       : 0.5 + 0.45 * std::sin(t * 0.5);
    surface.set(xfader_, static_cast<float>(xfader), now_us);

    surface.set(hi_, static_cast<float>(0.5 + 0.45 * std::sin(t * 0.31)), now_us);
    surface.set(mid_, static_cast<float>(triangle(t, 7.0)), now_us);

    const double filter = (t >= 16.0 && t < 20.0) ? triangle(t - 16.0, 2.0) : 0.5;
    surface.set(filter_, static_cast<float>(filter), now_us);

    surface.set(fader1_, 0.92f, now_us);
    surface.set(fader2_, static_cast<float>(0.6 + 0.3 * triangle(t, 9.0)), now_us);
}

}  // namespace svj
