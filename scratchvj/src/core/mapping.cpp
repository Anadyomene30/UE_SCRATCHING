#include "core/mapping.h"

namespace svj {

std::size_t MappingEngine::add(Mapping mapping) {
    mappings_.push_back(std::move(mapping));
    resolved_.push_back(kNoControl);
    states_.emplace_back();
    values_.push_back(0.0f);
    active_.push_back(false);
    return mappings_.size() - 1;
}

void MappingEngine::clear() {
    mappings_.clear();
    resolved_.clear();
    states_.clear();
    values_.clear();
    active_.clear();
}

std::vector<std::string> MappingEngine::resolve(const Surface& surface) {
    std::vector<std::string> unresolved;
    for (std::size_t i = 0; i < mappings_.size(); ++i) {
        if (mappings_[i].source.kind != SourceKind::Control) {
            resolved_[i] = kNoControl;
            continue;
        }
        const ControlIndex index = surface.find(mappings_[i].source.control_id);
        resolved_[i] = index;
        if (index == kNoControl) unresolved.push_back(mappings_[i].source.control_id);
    }
    return unresolved;
}

float MappingEngine::raw_source_value(const Surface& surface, const EngineInputs& inputs,
                                      std::size_t index) const {
    const Source& source = mappings_[index].source;
    const DeckSignals& deck = inputs.deck[source.deck > 1 ? 1 : source.deck];

    switch (source.kind) {
        case SourceKind::Control: {
            const ControlIndex control = resolved_[index];
            if (control == kNoControl) return 0.0f;
            return surface.at(control).value;
        }
        case SourceKind::DeckPosition:
            return deck.position_s;
        case SourceKind::DeckVelocity:
            return deck.velocity;
        case SourceKind::DeckAcceleration:
            return deck.acceleration;
        case SourceKind::DeckScratchRate:
            return deck.scratch_rate;
        case SourceKind::DeckConfidence:
            return deck.confidence;
        case SourceKind::Gesture:
            return (inputs.gesture_bits & source.gesture_bit) != 0 ? 1.0f : 0.0f;
    }
    return 0.0f;
}

void MappingEngine::evaluate(const Surface& surface, const EngineInputs& inputs, float dt_s) {
    for (std::size_t i = 0; i < mappings_.size(); ++i) {
        const Mapping& mapping = mappings_[i];

        // An unresolved control source is skipped rather than treated as zero: a
        // stale mapping file must not slam a parameter to the bottom of its range.
        const bool resolvable =
            mapping.source.kind != SourceKind::Control || resolved_[i] != kNoControl;

        if (!mapping.enabled || !resolvable) {
            active_[i] = false;
            states_[i].reset();
            continue;
        }

        active_[i] = true;
        const float raw = raw_source_value(surface, inputs, i);
        values_[i] = transform_apply_smoothed(mapping.transform, raw, dt_s, states_[i]);
    }
}

}  // namespace svj
