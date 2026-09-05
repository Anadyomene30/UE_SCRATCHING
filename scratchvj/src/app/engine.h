// scratchvj — the per-frame composition of everything the decks are made of.
//
// This is the application, minus its front end. A deck is a timecode tracker, a
// gesture tracker, a transport and a window of frames in video memory, and the
// engine is those two decks plus the surface, the mixer, the modulators, the
// effect rack and the mapping engine, advanced together in one defined order.
//
// It lives here rather than in main() for the reason every part of this project
// avoids living in main(): the order in which these are advanced is a design
// decision with consequences -- the modulators must see the position the
// transport produced, not the one the platter asked for -- and a decision worth
// making is a decision worth testing. It is also, verbatim, the code the real
// front end will run once there is a MIDI port and a GPU behind it, so writing
// it twice was never an option.
//
// Nothing here knows where its samples come from. The scripted simulation, a
// recorded take and a real pair of turntables are all the same to it.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/anchor.h"
#include "core/effect.h"
#include "core/framewindow.h"
#include "core/gestures.h"
#include "core/mapping.h"
#include "core/mixer.h"
#include "core/modulator.h"
#include "core/protocol.h"
#include "core/surface.h"
#include "core/timecode.h"
#include "core/transport.h"
#include "core/videocache.h"

namespace svj {

// Everything a deck is made of, wired together the way the real application will.
struct Deck {
    TimecodeTracker timecode;
    GestureTracker gestures;
    Transport transport;
    FrameWindow window;
    CacheHeader clip;
    std::string name;

    void configure(std::string label, double duration_s, std::uint32_t width,
                   std::uint32_t height, BlockFormat format, SignalProfile profile,
                   double bpm);

    // Feeds one decoder reading through the whole chain and returns the position
    // actually played, which is the platter's position after the transport has had
    // its say about loops, cues and slip.
    double advance(const DecoderSample& sample);
};

// What a front end asks a deck to do on a given frame. Buttons arrive as edges
// rather than as levels, because that is what a MIDI note is and what a script
// crossing a moment is; holding them as state is the front end's problem.
struct DeckCommands {
    bool loop_in = false;
    double loop_seconds = 2.0;
    bool loop_exit = false;
    bool slip_on = false;
    bool slip_off = false;
    bool cue_jump = false;
    int cue_index = 0;
};

// One frame's worth of input.
struct EngineFrame {
    double time_s = 0.0;
    float dt_s = 0.0f;
    std::uint64_t now_us = 0;
    DecoderSample deck_a;
    DecoderSample deck_b;
    DeckCommands commands_a;
};

// What downstream mappings see for a deck whose link has dropped.
//
// This is not a detail. A dropout on a wireless system is not a stopped platter
// and the timecode tracker is careful to say so -- but the mapping engine is
// still being asked, sixty times a second, how fast the platter is going, and
// every answer is a lie of a different kind. Freezing the last velocity leaves a
// glitch effect running on a platter that is not moving; zeroing it stops the
// effect dead the instant the radio stutters, which is louder than the fault.
enum class LinkLossPolicy : std::uint8_t {
    Hold,   // keep reporting the last values seen while the link was good
    Zero,   // snap every motion-derived signal to rest at once
    Decay,  // ease them to rest, so a brief dropout is not a visible event
};

// The motion-derived signals a deck contributes to the mapping engine, after the
// link-loss policy has been applied.
struct DeckMotion {
    float velocity = 0.0f;
    float scratch_rate = 0.0f;
    float acceleration = 0.0f;
};

// Applies `policy` to a deck that has lost its link. `previous` is what this deck
// reported on the last frame, `dt_s` the time since. Called only while the link
// is down; a locked deck reports what it actually measures.
DeckMotion apply_link_loss(LinkLossPolicy policy, const DeckMotion& previous, float dt_s);

class Engine {
public:
    // Builds the decks, the rack, the modulators and the default mappings. The
    // surface is left empty: whoever owns the controls declares them, then calls
    // bind().
    void configure(double bpm);

    // Resolves every mapping's control id against the surface as it now stands.
    // Returns the ids that matched nothing, which is a stale mapping file rather
    // than an error worth stopping for.
    std::vector<std::string> bind();

    void step(const EngineFrame& frame);

    Surface& surface() { return surface_; }
    const Surface& surface() const { return surface_; }
    Deck& deck_a() { return a_; }
    Deck& deck_b() { return b_; }
    const Deck& deck_a() const { return a_; }
    const Deck& deck_b() const { return b_; }

    const MappingEngine& mapping() const { return mapping_; }
    const ModulatorBank& modulators() const { return modulators_; }
    const EffectRack& rack() const { return rack_; }
    const CutDetector& cuts() const { return cuts_; }
    const Anchor& anchor() const { return anchor_; }
    MixWeights weights() const { return weights_; }
    double bpm() const { return bpm_; }

    void set_link_loss_policy(LinkLossPolicy policy) { policy_ = policy; }
    LinkLossPolicy link_loss_policy() const { return policy_; }

    // The surface as a schema, and the current state as a packet on the wire.
    SchemaPacket schema() const;
    StatePacket packet(std::uint64_t t_us, std::uint32_t schema_hash) const;

private:
    Surface surface_;
    Deck a_;
    Deck b_;
    MappingEngine mapping_;
    ModulatorBank modulators_;
    EffectRack rack_{3};
    CutDetector cuts_;
    Anchor anchor_;

    ControlIndex xfader_ = kNoControl;
    ControlIndex fader_a_ = kNoControl;
    ControlIndex fader_b_ = kNoControl;

    DeckMotion motion_a_;
    DeckMotion motion_b_;
    MixWeights weights_;
    double bpm_ = 120.0;
    LinkLossPolicy policy_ = LinkLossPolicy::Hold;
};

}  // namespace svj
