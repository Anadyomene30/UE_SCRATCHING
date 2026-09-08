// scratchvj — the registry of mapping destinations.
//
// A mapping names its destination as a string ("fx.1.mix", "deck.a.cue.3"),
// which is right for a file a performer edits by hand. What was wrong was the
// other end: the engine dispatched those strings by a hand-written chain of
// comparisons, so four targets did something and every other one -- including
// half of the demo's own mappings -- was silently decoration. This table is
// the whole vocabulary: a name resolves to an enumerator ONCE, at bind time,
// and an unknown name is reported the way an unknown control already was.
//
// Triggers are named as such. A pad mapped to a cue must jump on the PRESS
// and not sixty times a second while it is held; the engine derives the edge
// and the table says which destinations are edges.
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace svj {

enum class Dest : std::uint16_t {
    None = 0,

    // The mixer.
    MixTransition,        // informational: the crossfader itself reaches the mixer directly
    MixXfaderCurve,       // 0..1 quantised over Smooth, Linear, Sharp, Cut
    MixFaderCurve,
    MixXfaderReverse,
    MixFaderACurve, MixFaderBCurve, MixFaderAReverse, MixFaderBReverse,     // > 0.5 reverses the crossfader (hamster)
    MixFaderReverse,

    // Deck A's gaze into a sphere.
    DeckAYaw,
    DeckAPitch,
    DeckAFov,
    DeckAZoom,

    // Transport, per deck. Triggers.
    DeckACue1, DeckACue2, DeckACue3, DeckACue4, DeckACue5, DeckACue6, DeckACue7, DeckACue8,
    DeckBCue1, DeckBCue2, DeckBCue3, DeckBCue4, DeckBCue5, DeckBCue6, DeckBCue7, DeckBCue8,
    DeckALoopIn, DeckALoopOut, DeckALoopExit,
    DeckBLoopIn, DeckBLoopOut, DeckBLoopExit,
    DeckASlip, DeckBSlip,   // levels: held = slip on
    DeckALoadNext, DeckBLoadNext, OverlayLoadNext,  // triggers, served by the front end

    // The rack, by slot.
    Fx1Mix, Fx1Amount, Fx1Time, Fx1Feedback, Fx1Depth, Fx1Tone, Fx1On,
    Fx2Mix, Fx2Amount, Fx2Time, Fx2Feedback, Fx2Depth, Fx2Tone, Fx2On,
    Fx3Mix, Fx3Amount, Fx3Time, Fx3Feedback, Fx3Depth, Fx3Tone, Fx3On,
    // By type, wherever the effect sits: the demo's mappings and the mockup's
    // "filter knob = cutoff + blur" name effects, not slots.
    FxFilterAmount,       // the LowPass unit's amount
    FxKaleidoscopeRotation,
    FxGlitchAmount,
    FxBloomAmount,

    // The overlay layer.
    OverlayOpacity,
    OverlayEnabled,

    // The library, served by the front end. Triggers.
    LibraryNext, LibraryPrev, LibraryLoadA, LibraryLoadB, LibraryLoadOverlay,
};

struct DestinationSpec {
    const char* id;
    Dest dest;
    bool trigger;       // an edge, not a level
    const char* about;  // one line for the mapping screen
};

// Every destination, in a stable order for the interface to list.
const std::vector<DestinationSpec>& destination_catalogue();

// The enumerator for a name, or Dest::None. An OSC address ("/ue/shake") is
// not a local destination and resolves to None here on purpose: it goes out
// on the wire, not into the engine.
Dest resolve_destination(std::string_view id);
const DestinationSpec* describe_destination(Dest dest);
bool is_trigger(Dest dest);

// The cue index (0..7) a cue destination names, or -1.
int cue_of(Dest dest);
// Which rack slot (0..2) a slot destination names, or -1.
int fx_slot_of(Dest dest);

}  // namespace svj
