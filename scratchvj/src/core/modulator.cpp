#include "core/modulator.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kTwoPi = 6.28318530717958647692;

double fract(double v) {
    const double f = v - std::floor(v);
    return f < 0.0 ? f + 1.0 : f;
}

float shape_at(LfoShape shape, double phase) {
    switch (shape) {
        case LfoShape::Sine:
            return static_cast<float>(0.5 - 0.5 * std::cos(phase * kTwoPi));
        case LfoShape::Triangle:
            return static_cast<float>(phase < 0.5 ? phase * 2.0 : 2.0 - phase * 2.0);
        case LfoShape::RampUp:
            return static_cast<float>(phase);
        case LfoShape::RampDown:
            return static_cast<float>(1.0 - phase);
        case LfoShape::Square:
            return phase < 0.5 ? 0.0f : 1.0f;
    }
    return 0.0f;
}

}  // namespace

float lfo_value(const Lfo& lfo, double time_s, double beat_position) {
    // A tempo-synced LFO reads its phase from the beat position rather than
    // counting time. Derived phase cannot drift, and it follows the platter
    // backwards -- a counted one would not, and would break under a scratch.
    double phase = 0.0;
    if (lfo.beats > 0.0) {
        phase = fract(beat_position / lfo.beats + lfo.phase_offset);
    } else {
        phase = fract(time_s * lfo.rate_hz + lfo.phase_offset);
    }

    const float unit = shape_at(lfo.shape, phase);
    if (lfo.bipolar) return (unit * 2.0f - 1.0f) * lfo.depth;
    return lfo.centre + (unit - 0.5f) * lfo.depth;
}

float EnvelopeFollower::update(float input, float dt_s) {
    if (dt_s <= 0.0f) return value_;
    const float target = input * settings_.depth;
    const float tau_ms = target > value_ ? settings_.attack_ms : settings_.release_ms;
    if (tau_ms <= 0.0f) {
        value_ = target;
        return value_;
    }
    const float alpha = 1.0f - std::exp(-dt_s / (tau_ms * 0.001f));
    value_ += (target - value_) * alpha;
    return value_;
}

std::size_t ModulatorBank::add(Lfo lfo) {
    lfos_.push_back(lfo);
    slots_.push_back(Slot{Kind::Lfo, lfos_.size() - 1});
    values_.push_back(0.0f);
    return values_.size() - 1;
}

std::size_t ModulatorBank::add(Envelope envelope) {
    envelopes_.emplace_back(envelope);
    envelope_inputs_.push_back(0.0f);
    slots_.push_back(Slot{Kind::Envelope, envelopes_.size() - 1});
    values_.push_back(0.0f);
    return values_.size() - 1;
}

void ModulatorBank::clear() {
    slots_.clear();
    lfos_.clear();
    envelopes_.clear();
    envelope_inputs_.clear();
    values_.clear();
}

void ModulatorBank::set_envelope_input(std::size_t index, float value) {
    if (index >= slots_.size()) return;
    const Slot& slot = slots_[index];
    if (slot.kind != Kind::Envelope) return;
    envelope_inputs_[slot.index] = value;
}

void ModulatorBank::update(double time_s, double beat_position, float dt_s) {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const Slot& slot = slots_[i];
        if (slot.kind == Kind::Lfo) {
            values_[i] = lfo_value(lfos_[slot.index], time_s, beat_position);
        } else {
            values_[i] = envelopes_[slot.index].update(envelope_inputs_[slot.index], dt_s);
        }
    }
}

}  // namespace svj
