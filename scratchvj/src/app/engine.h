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
#include "core/destinations.h"
#include "core/effect.h"
#include "core/framewindow.h"
#include "core/gestures.h"
#include "core/library.h"
#include "core/matrix.h"
#include "core/mapping.h"
#include "core/mesh.h"
#include "core/mixer.h"
#include "core/modulator.h"
#include "core/playback.h"
#include "core/protocol.h"
#include "core/spectrum.h"
#include "core/sphere.h"
#include "core/surface.h"
#include "core/timecode.h"
#include "core/transport.h"
#include "core/videocache.h"
#include "core/warp.h"

namespace svj {

// Everything a deck is made of, wired together the way the real application will.
struct Deck {
    TimecodeTracker timecode;
    GestureTracker gestures;
    DeckClock clock;
    Transport transport;
    FrameWindow window;
    CacheHeader clip;
    std::string name;

    // What the clock resolved on the last advance(): the position actually
    // shown, its signed velocity, and whether the clip is running backwards or
    // parked. A free-running deck has no platter to ask, so this is where
    // anything downstream reads its motion from.
    ClockOutput played;

    void configure(std::string label, double duration_s, std::uint32_t width,
                   std::uint32_t height, BlockFormat format, SignalProfile profile,
                   double bpm);

    // Feeds one decoder reading through the whole chain and returns the position
    // actually played, which is the platter's position after the transport has had
    // its say about loops, cues and slip.
    double advance(const DecoderSample& sample);

    // Advances a deck that has no platter at all: the overlay layer, or a deck a
    // performer has switched to a free or tempo-locked source. There is no
    // timecode to submit and no gesture to track -- the clock is the whole input.
    double advance_free(double time_s);

    // Points the deck at a real analysed clip, replacing whatever configure()
    // fabricated. Everything that depends on the clip's duration or frame count
    // -- transport, clock, VRAM window -- is rebuilt; the timecode tracker is
    // not, because the platter does not change when the record does.
    //
    // `time_s` is the clock the deck will be advanced with. It matters: the
    // free-running clock is a closed form of absolute time, so a clip rebuilt
    // at time zero and first advanced at t = 300 s would open three hundred
    // seconds in. `keep_position` holds the played position across the load,
    // for the case where the SAME clip comes back with a different projection.
    void load(const CacheHeader& header, std::string label, double bpm, double time_s = 0.0,
              bool keep_position = false);

    // --- the deck as a player ------------------------------------------------
    // A deck is a video player first and a turntable second. These three are
    // what a transport row does: they leave the platter (the free clock takes
    // over from where the picture is) and they never move the picture except
    // where the word says so. Play restores the rate pause put away, so a deck
    // paused at -2x resumes at -2x; a deck never played runs at 1x.
    void play(double time_s);
    void pause(double time_s);
    void stop(double time_s);   // pause, and back to the head
    // Running on its own clock at a non-zero rate. A platter deck is not
    // "playing" in this sense: the record decides, not a button.
    bool playing() const;

    // A hand on the picture: the position bar being dragged. Grab remembers
    // where the deck was taking its position from, so that letting go returns
    // THERE -- a playing deck resumes, a platter deck is handed back through the
    // takeover policy. The clock alone forgets, which handed every released
    // scrub to the turntable, whether or not one was driving.
    void grab(double position_s, double time_s);
    void scrub(double position_s, double time_s);
    void release(double time_s);

    double resume_rate = 1.0;
    DeckSource source_before_hand = DeckSource::FreeRun;
    // The frame window's budget, set by the engine from the desk's settings.
    std::uint64_t window_budget_bytes = 256ull << 20;
};

// What a front end asks a deck to do on a given frame. Buttons arrive as edges
// rather than as levels, because that is what a MIDI note is and what a script
// crossing a moment is; holding them as state is the front end's problem.
struct DeckCommands {
    bool loop_in = false;
    double loop_seconds = 2.0;
    bool loop_out = false;   // closes the loop here, for a mapped "loop out" button
    bool loop_exit = false;
    bool slip_on = false;
    bool slip_off = false;
    bool cue_jump = false;
    bool cue_set = false;    // set cue_index where the deck is now
    bool cue_clear = false;
    int cue_index = 0;
    // A loop of that many beats from the (quantised) position; zero asks
    // nothing. What a pad in "loops" mode and the Elite's loop encoder send.
    double auto_loop_beats = 0.0;
    // A jump of that many beats, negative backwards; zero asks nothing.
    double beat_jump_beats = 0.0;

    bool any() const {
        return loop_in || loop_out || loop_exit || slip_on || slip_off || cue_jump ||
               cue_set || cue_clear || auto_loop_beats != 0.0 || beat_jump_beats != 0.0;
    }
};

// One frame's worth of input.
struct EngineFrame {
    double time_s = 0.0;
    float dt_s = 0.0f;
    std::uint64_t now_us = 0;
    DecoderSample deck_a;
    DecoderSample deck_b;
    DeckCommands commands_a;
    DeckCommands commands_b;
};

// What a mapping asked of the FRONT END this step: loads and library moves are
// file I/O and list navigation the engine has no business doing itself, so it
// records the ask and whoever owns the window serves it at the frame boundary
// -- the same door a click on the library goes through.
struct EngineRequests {
    bool load_next_a = false;
    bool load_next_b = false;
    bool load_next_overlay = false;
    int library_step = 0;  // +1 next row, -1 previous, summed over the step
    bool library_load_a = false;
    bool library_load_b = false;
    bool library_load_overlay = false;

    bool any() const {
        return load_next_a || load_next_b || load_next_overlay || library_step != 0 ||
               library_load_a || library_load_b || library_load_overlay;
    }
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
    // Whether configure() fabricates the demo's clips and library.
    //
    // The demo was the scaffolding that let the whole instrument be built
    // before there was any hardware or any analysed video: two decks with
    // invented names, one of them 360, and a library of clips that do not
    // exist. That scaffolding must not be what a performer sees on launch --
    // an instrument whose decks are full of files you do not own, one of them
    // spherical, reads as a simulation and hides what the software actually
    // does. So it is a choice now, and the front end says no.
    enum class DemoContent : std::uint8_t { No, Yes };

    void configure(double bpm, DemoContent demo = DemoContent::Yes);

    // Resolves every mapping's control id against the surface as it now stands.
    // Returns the ids that matched nothing -- control ids AND destination
    // names -- which is a stale mapping file rather than an error worth
    // stopping for. A mapping to an unknown destination used to be silently
    // decoration; now it is a line in this list.
    std::vector<std::string> bind();

    // Replaces the mappings wholesale (a mapping.json, a preset) and binds.
    std::vector<std::string> install_mappings(std::vector<Mapping> mappings);

    // What the mixer's switches say: curves and reverse, for the crossfader
    // and for the channel faders separately. Written by the interface, by a
    // mapping (the Elite's own curve buttons, once measured), and by settings.
    MixSettings& mix_settings() { return mix_; }
    const MixSettings& mix_settings() const { return mix_; }

    // Asks the last step() raised for the front end, and clears them.
    EngineRequests take_requests();

    void step(const EngineFrame& frame);

    Surface& surface() { return surface_; }
    const Surface& surface() const { return surface_; }
    Deck& deck_a() { return a_; }
    Deck& deck_b() { return b_; }
    const Deck& deck_a() const { return a_; }
    const Deck& deck_b() const { return b_; }

    // The third layer: a deck with no platter, running over the two below it.
    Deck& overlay() { return overlay_; }
    const Deck& overlay() const { return overlay_; }
    Layer& overlay_layer() { return overlay_layer_; }
    const Layer& overlay_layer() const { return overlay_layer_; }
    StackWeights stack() const { return stack_; }

    // The clip library and the play queue. They belong to the engine rather than
    // to a front end because a pad on the mixer loads the next clip just as a
    // click does, and both have to reach the same list.
    Library& library() { return library_; }
    const Library& library() const { return library_; }
    Queue& queue() { return queue_; }
    const Queue& queue() const { return queue_; }
    // The pad banks: what a pad in "clips" mode loads. Here for the reason
    // the library is: a hardware pad and a click on screen reach one table.
    Matrix& matrix() { return matrix_; }
    const Matrix& matrix() const { return matrix_; }

    // Deck A's gaze into a 360 clip. Yaw and pitch arrive through the mapping
    // engine every step (the EQ knobs in the demo rig); projection, field of
    // view and zoom are performance settings the interface writes, like the
    // overlay. The view is deck state and NOT clip state: loading a new clip
    // must not snap the gaze.
    SphereView& view_a() { return view_a_; }
    const SphereView& view_a() const { return view_a_; }

    // Output geometry: the corner pin and the mask, owned here for the same
    // reason the overlay layer is -- the performer sets them, every front end
    // and every output (screen, Spout, NDI) must see the same ones.
    CornerPin& pin() { return pin_; }
    const CornerPin& pin() const { return pin_; }

    // The warp grid, for surfaces a homography cannot describe. Off until
    // asked for: the pin alone is right for the ordinary flat wall, and a grid
    // switched on by default would put sixteen handles between a performer and
    // the four they actually need.
    WarpMesh& mesh() { return mesh_; }
    const WarpMesh& mesh() const { return mesh_; }
    bool mesh_enabled() const { return mesh_enabled_; }
    void set_mesh_enabled(bool on) { mesh_enabled_ = on; }
    Mask& mask() { return mask_; }
    const Mask& mask() const { return mask_; }

    const MappingEngine& mapping() const { return mapping_; }
    // Mutable because mappings are edited at run time: MIDI learn binds them, a
    // preset replaces them, and the interface will let one be added by hand.
    // Anything added after configure() needs bind() called again to resolve its
    // control ids against the surface.
    MappingEngine& mapping() { return mapping_; }
    const ModulatorBank& modulators() const { return modulators_; }

    // Live audio for the reactive bands. Called from whoever owns the input --
    // an audio callback once miniaudio exists, the demo script until then. The
    // engine does not open a device and never will: `core/` has no dependency,
    // and an instrument that grabs the sound card in follower mode would stop
    // Serato from working. See the roadmap's two audio modes.
    //
    // Nothing calls this in a set where no audio arrives, and that is fine: the
    // bands decay to zero and every AudioBand mapping simply reads 0.
    void analyse_audio(const float* samples, std::size_t count);
    const SpectrumAnalyser& spectrum() const { return spectrum_; }
    // Editable: EFFETS loads, clears, links and turns the slots. The rack is
    // the performer's, like the overlay layer.
    EffectRack& rack() { return rack_; }
    const EffectRack& rack() const { return rack_; }
    const CutDetector& cuts() const { return cuts_; }
    const Anchor& anchor() const { return anchor_; }
    // Places the anchor NOW: this position on deck A's control record means
    // this position in its clip. The button the roadmap's verification step 3
    // needs, which used to exist only as a configure-time call with zeros.
    void anchor_now(double now_s);

    // Video memory each deck may hold (the frame window's budget). A setting:
    // 4K equirect on a modest card needs a smaller window. Applied to the
    // decks now and to every later load.
    void set_window_budget(std::uint64_t bytes);
    std::uint64_t window_budget() const { return window_budget_; }
    MixWeights weights() const { return weights_; }
    // The crossfader as the transitions read it: 0 on A, 1 on B, reverse
    // applied. The weights are the curve's; this is the position.
    float crossfader_position() const { return xfade_position_; }
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
    Deck overlay_;
    Layer overlay_layer_;
    StackWeights stack_;
    SphereView view_a_;
    CornerPin pin_;
    WarpMesh mesh_;
    bool mesh_enabled_ = false;
    Mask mask_;
    Library library_;
    Queue queue_;
    Matrix matrix_;
    std::uint64_t window_budget_ = 256ull << 20;
    MappingEngine mapping_;
    ModulatorBank modulators_;
    // 1024 at 48 kHz: 21 ms of resolution and of latency together, inside the
    // 30 ms the roadmap allows for the whole hand-to-screen chain.
    SpectrumAnalyser spectrum_{1024, 48000.0, SpectrumSettings{}};
    double audio_silent_s_ = 0.0;
    EffectRack rack_{3};
    CutDetector cuts_;
    Anchor anchor_;

    ControlIndex xfader_ = kNoControl;
    ControlIndex fader_a_ = kNoControl;
    ControlIndex fader_b_ = kNoControl;
    MixSettings mix_;

    // Per mapping, resolved once at bind(): where it goes, and whether its
    // trigger was already above the threshold last step (an edge fires once).
    std::vector<Dest> resolved_dest_;
    std::vector<bool> trigger_high_;
    EngineRequests requests_;

    void dispatch(std::size_t index, Dest dest, float value, DeckCommands& a, DeckCommands& b);
    void configure_rack_and_mappings();

    DeckMotion motion_a_;
    DeckMotion motion_b_;
    MixWeights weights_;
    float xfade_position_ = 0.5f;
    double bpm_ = 120.0;
    LinkLossPolicy policy_ = LinkLossPolicy::Hold;
};

}  // namespace svj
