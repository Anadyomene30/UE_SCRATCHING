// scratchvj — LFOs and envelope followers.
//
// A parameter has to be able to move on its own, and to follow the music. These
// are sources for the mapping engine exactly like a knob is, which is what makes
// them cheap: nothing downstream needs to know where a value came from.
//
// Tempo sync costs almost nothing here because both halves already exist: the
// beat grid comes out of the analysis pass and the exact position comes out of
// the timecode, so an LFO's phase is DERIVED rather than counted. That matters --
// a counted phase drifts, and worse, it cannot be scratched. Deriving it means a
// tempo-synced LFO follows the platter backwards too.
#pragma once

#include <cstdint>
#include <vector>

namespace svj {

enum class LfoShape : std::uint8_t {
    Sine,
    Triangle,
    RampUp,
    RampDown,
    Square,
};

struct Lfo {
    LfoShape shape = LfoShape::Sine;

    // Period. When `beats` is above zero the LFO is tempo-synced and its phase is
    // derived from the beat position; otherwise it free-runs at `rate_hz`.
    double beats = 0.0;
    double rate_hz = 1.0;

    double phase_offset = 0.0;  // 0..1
    float depth = 1.0f;
    float centre = 0.5f;
    bool bipolar = false;  // when true the output swings around zero instead
};

// Follows a signal with separate rise and fall times: fast to react, slow to let
// go, which is what makes an audio-reactive parameter look musical rather than
// twitchy.
struct Envelope {
    float attack_ms = 5.0f;
    float release_ms = 180.0f;
    float depth = 1.0f;
};

// Evaluates one LFO. `beat_position` is the position in beats, which the caller
// derives from the beat grid and the played position.
float lfo_value(const Lfo& lfo, double time_s, double beat_position);

class EnvelopeFollower {
public:
    explicit EnvelopeFollower(Envelope settings = {}) : settings_(settings) {}

    float update(float input, float dt_s);
    float value() const { return value_; }
    void reset() { value_ = 0.0f; }

    void configure(Envelope settings) { settings_ = settings; }

private:
    Envelope settings_;
    float value_ = 0.0f;
};

// The modulators a deck offers to the mapping engine, evaluated together so the
// engine only ever sees a flat array of numbers.
class ModulatorBank {
public:
    std::size_t add(Lfo lfo);
    std::size_t add(Envelope envelope);

    void update(double time_s, double beat_position, float dt_s);

    // Sets the input an envelope follows, before the next update().
    void set_envelope_input(std::size_t index, float value);

    const std::vector<float>& values() const { return values_; }
    std::size_t size() const { return values_.size(); }
    void clear();

private:
    enum class Kind : std::uint8_t { Lfo, Envelope };

    struct Slot {
        Kind kind = Kind::Lfo;
        std::size_t index = 0;
    };

    std::vector<Slot> slots_;
    std::vector<Lfo> lfos_;
    std::vector<EnvelopeFollower> envelopes_;
    std::vector<float> envelope_inputs_;
    std::vector<float> values_;
};

}  // namespace svj
