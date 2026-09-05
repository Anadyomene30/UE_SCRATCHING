// scratchvj — the mapping engine.
//
// This is the answer to "I want to drive more than the crossfader". Any source
// (a physical control, or a quantity derived from the platter) reaches any
// destination (a local parameter, or an OSC address on the Unreal machine)
// through its own transform. One control may feed several destinations with
// different curves.
//
// The engine evaluates and reports values; it deliberately does not know how to
// apply them. Dispatch belongs to the layer that owns effect racks and sockets,
// which keeps this whole file testable without any of them.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/curve.h"
#include "core/surface.h"

namespace svj {

enum class SourceKind : std::uint8_t {
    Control,           // a fader, knob, button, encoder or pad
    DeckPosition,      // seconds on the control record
    DeckVelocity,      // signed speed ratio
    DeckAcceleration,
    DeckScratchRate,   // direction reversals per second
    DeckConfidence,
    Gesture,           // a bit from the gesture bitfield, as 0 or 1
    Modulator,         // an LFO or envelope follower, by index
    AudioBand,         // an audio-reactive frequency band, by index
};

enum class DestinationKind : std::uint8_t {
    Local,  // "fx.a.0.mix", "deck.a.yaw", "mix.transition"
    Osc,    // "/ue/shake" — forwarded to the Unreal machine
};

struct Source {
    SourceKind kind = SourceKind::Control;
    std::string control_id;              // when kind == Control
    std::uint8_t deck = 0;               // 0 = A, 1 = B, for the deck sources
    std::uint32_t gesture_bit = 0;       // when kind == Gesture
    std::uint16_t index = 0;             // when kind == Modulator or AudioBand
};

struct Destination {
    DestinationKind kind = DestinationKind::Local;
    std::string target;
};

struct Mapping {
    std::string name;
    Source source;
    Transform transform;
    Destination destination;
    bool enabled = true;
};

// Per-deck inputs to the engine, filled from the timecode decoder and the gesture
// tracker each block.
struct DeckSignals {
    float position_s = 0.0f;
    float velocity = 0.0f;
    float acceleration = 0.0f;
    float scratch_rate = 0.0f;
    float confidence = 0.0f;
};

struct EngineInputs {
    DeckSignals deck[2];
    std::uint32_t gesture_bits = 0;

    // Flat arrays the engine reads by index. Keeping them as plain spans is what
    // lets modulators and audio reactivity be sources without the engine knowing
    // anything about LFOs or spectra.
    const float* modulators = nullptr;
    std::size_t modulator_count = 0;
    const float* bands = nullptr;
    std::size_t band_count = 0;
};

class MappingEngine {
public:
    // Returns the index of the new mapping.
    std::size_t add(Mapping mapping);
    void clear();

    std::size_t size() const { return mappings_.size(); }
    const Mapping& at(std::size_t index) const { return mappings_[index]; }
    Mapping& mutable_at(std::size_t index) { return mappings_[index]; }

    // Binds every Control source to its index in `surface`. Call after loading a
    // mapping file, and again whenever the surface layout changes. Returns the ids
    // that matched no control, so the UI can flag a stale mapping file rather than
    // silently dropping rows.
    std::vector<std::string> resolve(const Surface& surface);

    // Evaluates every enabled mapping. `dt_s` advances the smoothing filters.
    void evaluate(const Surface& surface, const EngineInputs& inputs, float dt_s);

    // Value of mapping `index` from the last evaluate(). Zero for disabled rows.
    float value(std::size_t index) const { return values_[index]; }

    // True when the last evaluate() actually ran this mapping.
    bool active(std::size_t index) const { return active_[index]; }

private:
    float raw_source_value(const Surface& surface, const EngineInputs& inputs,
                           std::size_t index) const;

    std::vector<Mapping> mappings_;
    std::vector<ControlIndex> resolved_;  // parallel; kNoControl for non-Control sources
    std::vector<TransformState> states_;
    std::vector<float> values_;
    std::vector<bool> active_;
};

}  // namespace svj
