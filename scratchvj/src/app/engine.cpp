#include "app/engine.h"

#include "core/compose.h"

#include <algorithm>

namespace svj {
namespace {

// A control mapped straight through, which is most of them.
Mapping control_mapping(std::string name, std::string id, std::string target, float lo,
                        float hi) {
    Mapping m;
    m.name = std::move(name);
    m.source.kind = SourceKind::Control;
    m.source.control_id = std::move(id);
    m.transform.out_lo = lo;
    m.transform.out_hi = hi;
    m.destination.target = std::move(target);
    return m;
}

// What a deck reports while its link is good.
DeckMotion measured(const Deck& deck) {
    DeckMotion motion;
    // The clock's velocity rather than the platter's: they agree on a deck that
    // follows a turntable, and on one that does not, only the clock knows.
    motion.velocity = static_cast<float>(deck.played.velocity);
    motion.scratch_rate = deck.gestures.scratch_rate();
    motion.acceleration = deck.gestures.acceleration();
    return motion;
}

float value_of(const Surface& surface, ControlIndex index) {
    return index == kNoControl ? 0.0f : surface.at(index).value;
}

// A control nobody has touched, and what the MIX should make of it.
//
// The ghost rule says never to DISPLAY an invented position for an absolute
// potentiometer: at launch its real place is unknown, and drawing it at zero
// would be a lie. But the mixer still has to compute something, and there the
// two choices are not symmetric. Reading an untouched channel fader as CLOSED
// makes a fresh instrument show nothing at all -- load a clip, see black, and
// conclude the software is broken. Reading it as OPEN is wrong only until the
// fader is first moved, and it is wrong in the direction where the picture is
// there. A mixer's faders are up when you walk to it.
//
// The control keeps drawing as a ghost either way: this is what the mix
// assumes, not what the interface claims.
float value_or(const Surface& surface, ControlIndex index, float when_unknown) {
    if (index == kNoControl) return when_unknown;
    const Control& control = surface.at(index);
    return control.known ? control.value : when_unknown;
}

}  // namespace

// --- Deck --------------------------------------------------------------------

void Deck::configure(std::string label, double duration_s, std::uint32_t width,
                     std::uint32_t height, BlockFormat format, SignalProfile profile,
                     double bpm) {
    name = std::move(label);
    clip.width = width;
    clip.height = height;
    clip.fps_num = 60;
    clip.fps_den = 1;
    clip.format = format;
    clip.frame_count = static_cast<std::uint32_t>(duration_s * 60.0);
    // The fabricated clip's own claim about its shape, so the demo's 360 deck
    // reads as one through the same flag a real cache carries. The interface
    // asks the flag and nothing else; it no longer re-derives this from the
    // aspect ratio.
    clip.flags = width == height * 2 ? kCacheEquirect : std::uint16_t{0};

    TimecodeConfig config;
    config.profile = profile;
    config.mode = TransportMode::Relative;
    timecode = TimecodeTracker(config);

    transport.configure(clip.duration_s(), BeatGrid{bpm, 0.0});
    clock.configure(clip.duration_s(), bpm);

    WindowConfig window_config;
    window_config.budget_bytes = window_budget_bytes;
    window.configure(clip.frame_count, block_bytes_per_frame(width, height, format),
                     window_config);
}

double Deck::advance(const DecoderSample& sample) {
    const TimecodeState& state = timecode.submit(sample);
    gestures.update(sample.time_s, state.velocity, state.confidence);
    // Source, then transport, then fold. The clip's own ends are applied last:
    // a user loop straddling the end of the clip would otherwise be folded away
    // before the transport ever saw it. See core/playback.h.
    const SourceReading reading =
        clock.read_source(sample.time_s, state.position_s, state.velocity);
    const double mapped = transport.map(reading.position_s);
    played = clock.resolve(mapped, reading.rate);

    window.update(clip.frame_at(played.position_s),
                  static_cast<float>(played.velocity));
    return played.position_s;
}

void Deck::load(const CacheHeader& header, std::string label, double bpm, double time_s,
                bool keep_position) {
    name = std::move(label);
    const double held_s = played.position_s;
    clip = header;
    transport.configure(clip.duration_s(), BeatGrid{bpm, 0.0});

    // The clock is rebuilt for the new duration, but what the deck IS survives
    // the record changing: its source, its play mode, its takeover policy and
    // its beat count are the performer's settings, not the clip's. Without
    // this the overlay -- tempo-locked, bouncing, deaf to the platter --
    // silently became a looping timecode deck the moment a logo was dropped
    // on it, and the mask started following the left turntable.
    const DeckSource source = clock.source();
    const ClipPlayMode mode = clock.mode();
    const TakeoverMode takeover = clock.takeover();
    const double beats = clock.beats_per_cycle();
    const double rate = clock.rate();
    clock.configure(clip.duration_s(), bpm);
    clock.set_source(source, time_s);
    clock.set_mode(mode);
    clock.set_takeover(takeover);
    clock.set_beats_per_cycle(beats, time_s);
    clock.set_rate(rate, time_s);
    // Seek AFTER the rate, at the caller's clock: the origin the closed form
    // runs from is (this position, this time), so the first advance() after
    // the load finds the picture at the head, not at wall-clock-mod-duration.
    const double start_s = keep_position ? std::min(held_s, clip.duration_s()) : 0.0;
    clock.seek(start_s, time_s);

    WindowConfig window_config;
    window_config.budget_bytes = window_budget_bytes;
    window.configure(clip.frame_count,
                     block_bytes_per_frame(clip.width, clip.height, clip.format),
                     window_config);
    played = ClockOutput{};
    played.position_s = start_s;
}

// --- the deck as a player ----------------------------------------------------

void Deck::play(double time_s) {
    const DeckSource source = clock.source();
    if (source == DeckSource::Timecode || source == DeckSource::Hand) {
        clock.set_source(DeckSource::FreeRun, time_s);
    }
    clock.set_rate(resume_rate != 0.0 ? resume_rate : 1.0, time_s);
}

void Deck::pause(double time_s) {
    const DeckSource source = clock.source();
    if (source == DeckSource::Timecode || source == DeckSource::Hand) {
        // set_source holds the position the deck was last resolved at, so the
        // frame that was showing is the one that stays.
        clock.set_source(DeckSource::FreeRun, time_s);
    }
    if (clock.rate() != 0.0) resume_rate = clock.rate();
    clock.set_rate(0.0, time_s);
}

void Deck::stop(double time_s) {
    pause(time_s);
    clock.seek(0.0, time_s);
    played.position_s = 0.0;
}

bool Deck::playing() const {
    const DeckSource source = clock.source();
    return (source == DeckSource::FreeRun || source == DeckSource::TempoLocked) &&
           clock.effective_rate() != 0.0;
}

void Deck::grab(double position_s, double time_s) {
    if (clock.source() != DeckSource::Hand) source_before_hand = clock.source();
    clock.grab(position_s, time_s);
}

void Deck::scrub(double position_s, double time_s) { clock.scrub(position_s, time_s); }

void Deck::release(double time_s) {
    if (clock.source() != DeckSource::Hand) return;
    if (source_before_hand == DeckSource::Timecode) {
        // Back to the platter, offset so this exact frame is the one that stays.
        clock.hand_over_to_timecode(timecode.state().position_s, time_s);
    } else {
        clock.set_source(source_before_hand, time_s);
    }
}

double Deck::advance_free(double time_s) {
    const SourceReading reading = clock.read_source(time_s, 0.0, 0.0f);
    const double mapped = transport.map(reading.position_s);
    played = clock.resolve(mapped, reading.rate);

    window.update(clip.frame_at(played.position_s),
                  static_cast<float>(played.velocity));
    return played.position_s;
}

// --- the link-loss policy ----------------------------------------------------

DeckMotion apply_link_loss(LinkLossPolicy policy, const DeckMotion& previous, float dt_s) {
    switch (policy) {
        case LinkLossPolicy::Zero:
            return DeckMotion{};

        case LinkLossPolicy::Decay:
            // TODO: ease `previous` towards rest over dt_s. Left unwritten on
            // purpose -- the time constant is a performance decision, not a
            // mechanical one -- and falling back to Hold until it is, which is
            // why selecting Decay currently behaves exactly like Hold.
            return previous;

        case LinkLossPolicy::Hold:
        default:
            (void)dt_s;
            return previous;
    }
}

// --- Engine ------------------------------------------------------------------

void Engine::configure(double bpm, DemoContent demo) {
    bpm_ = bpm;
    const bool fabricate = demo == DemoContent::Yes;

    if (fabricate) {
        a_.configure("DECK A  tokyo_nightdrive_360", 240.0, 3840, 1920, BlockFormat::BC1,
                     SignalProfile::Wireless, bpm);
        b_.configure("DECK B  grain_loop_04", 12.0, 1920, 1080, BlockFormat::BC7,
                     SignalProfile::Wireless, bpm);
    } else {
        // Empty decks, and 16:9 rather than 2:1: a deck with no clip must not
        // claim to hold a sphere. The wells say what they are waiting for.
        a_.configure("", 0.0, 1920, 1080, BlockFormat::BC1, SignalProfile::Wireless, bpm);
        b_.configure("", 0.0, 1920, 1080, BlockFormat::BC1, SignalProfile::Wireless, bpm);
    }
    // The third layer. It has no platter by construction, so it is the first
    // thing in the engine that exercises core/playback: tempo-locked, one pass
    // per four beats, bouncing rather than cutting back to its head -- an eight
    // second texture has a visible seam on a wrap and none on a bounce.
    overlay_.configure(fabricate ? "INCRUST  city_grid_mask" : "", fabricate ? 8.0 : 0.0, 1280,
                       720, BlockFormat::BC7, SignalProfile::Wireless, bpm);
    overlay_.clock.set_source(DeckSource::TempoLocked, 0.0);
    overlay_.clock.set_beats_per_cycle(4.0, 0.0);
    overlay_.clock.set_mode(ClipPlayMode::PingPong);
    // A stray hand on a platter must never capture the layer holding the mask.
    overlay_.clock.set_takeover(TakeoverMode::Ignore);
    // Off until something is actually on it: a layer compositing an empty
    // texture at 45% is a dark wash over the programme for no reason.
    overlay_layer_.enabled = fabricate;
    overlay_layer_.opacity = 0.45f;
    overlay_layer_.blend = BlendMode::Screen;

    // The library the demo browses, and the queue standing behind the decks. Made
    // here rather than in a front end for the reason the decks are: a pad on the
    // mixer loads the next clip exactly as a click does, so both have to be
    // looking at the same list. The entries stand in for an analysis pass that
    // does not exist yet, which is why one of them is deliberately mid-analysis
    // and therefore not playable -- an unanalysed clip has to be visibly not
    // loadable, or the first thing the instrument does on stage is stutter.
    library_ = Library{};
    queue_ = Queue{};
    if (!fabricate) {
        mesh_.reset(3, 3);
        anchor_.set(0.0, 0.0, 0.0, 0);
        configure_rack_and_mappings();
        return;
    }
    const int all = library_.create_crate("Tous les clips");
    const int spherical = library_.create_crate("360\xC2\xB0");
    const int loops = library_.create_crate("Loops & textures");

    const auto add = [&](const char* path, const char* name, double duration, unsigned w,
                         unsigned h, double fps, bool equirect, AnalysisState state,
                         float progress) {
        ClipEntry entry;
        entry.path = path;
        entry.name = name;
        entry.duration_s = duration;
        entry.width = w;
        entry.height = h;
        entry.fps = fps;
        entry.equirect = equirect;
        entry.bpm = bpm;
        const ClipId id = library_.add(entry);
        library_.set_state(id, state, progress);
        library_.add_to_crate(all, id);
        if (equirect) library_.add_to_crate(spherical, id);
        if (duration < 60.0) library_.add_to_crate(loops, id);
        return id;
    };

    add("clips/tokyo_nightdrive_360.mp4", "tokyo_nightdrive_360.mp4", 252.0, 3840, 1920, 60.0,
        true, AnalysisState::Ready, 1.0f);
    add("clips/grain_loop_04.mov", "grain_loop_04.mov", 12.0, 1920, 1080, 60.0, false,
        AnalysisState::Ready, 1.0f);
    add("clips/rooftop_pan_4k.mp4", "rooftop_pan_4k.mp4", 158.0, 3840, 2160, 30.0, false,
        AnalysisState::Analysing, 0.62f);
    const ClipId neon = add("clips/neon_alley_360.mp4", "neon_alley_360.mp4", 184.0, 3840, 1920,
                            30.0, true, AnalysisState::Ready, 1.0f);
    const ClipId vhs = add("clips/vhs_static_b.mov", "vhs_static_b.mov", 31.0, 1920, 1080, 30.0,
                           false, AnalysisState::Ready, 1.0f);
    const ClipId crowd = add("clips/crowd_slowmo.mp4", "crowd_slowmo.mp4", 107.0, 1920, 1080,
                             120.0, false, AnalysisState::Ready, 1.0f);

    queue_.push(neon, DeckTarget::A);
    queue_.push(vhs, DeckTarget::B);
    queue_.push(crowd);

    mesh_.reset(3, 3);

    a_.transport.set_cue(0, 0.0, 1);
    a_.transport.set_cue(1, 30.0, 2);
    a_.transport.set_cue(2, 96.0, 3);

    anchor_.set(0.0, 0.0, 0.0, 0);
    configure_rack_and_mappings();
}

// The instrument itself: the rack a set starts from, the two modulators, and
// the default routing. Not demo content -- these are what the software IS,
// and they are set up whether or not any clip has been invented.
void Engine::configure_rack_and_mappings() {
    // A tempo-synced LFO and an envelope following how hard the platter is being
    // worked. Both reach the mapping engine as plain numbers.
    modulators_.clear();
    Lfo sweep;
    sweep.shape = LfoShape::Triangle;
    sweep.beats = 2.0;
    modulators_.add(sweep);
    Envelope follower;
    follower.attack_ms = 20.0f;
    follower.release_ms = 400.0f;
    modulators_.add(follower);

    rack_.load(0, EffectType::Delay);
    rack_.at(0).sync.tempo = true;
    rack_.at(0).sync.beats = 0.5;
    rack_.at(0).shared.mix = 0.62f;
    rack_.at(0).shared.feedback = 0.55f;
    rack_.load(1, EffectType::LowPass);
    rack_.at(1).shared.mix = 0.41f;
    // Unlinked on purpose, to show the one state where the two domains diverge.
    // Kaleidoscope rather than slit scan in the demo, for one honest reason:
    // slit scan reads the clip at several positions and nothing draws it yet,
    // so it would be a knob that visibly does nothing. Slot 0's delay is left
    // exactly that way on purpose, so the state IS visible somewhere -- one
    // undrawn effect is a note about what is coming, three is a broken rack.
    rack_.load(2, EffectType::Kaleidoscope);
    rack_.at(2).shared.mix = 0.77f;
    rack_.at(2).unlink();
    rack_.at(2).audio_override.mix = 0.0f;

    mapping_.clear();
    mapping_.add(control_mapping("ch1.eq.hi -> deck A yaw 360", "ch1.eq.hi", "deck.a.yaw",
                                 -180.0f, 180.0f));
    mapping_.add(control_mapping("ch1.eq.mid -> deck A pitch 360", "ch1.eq.mid",
                                 "deck.a.pitch", -90.0f, 90.0f));

    Mapping filter =
        control_mapping("ch1.filter -> cutoff + flou", "ch1.filter", "fx.a.filter", 0.0f, 1.0f);
    filter.transform.deadzone = 0.08f;
    mapping_.add(filter);

    Mapping glitch;
    glitch.name = "deck A vitesse -> glitch";
    glitch.source.kind = SourceKind::DeckVelocity;
    glitch.transform.in_lo = -8.0f;
    glitch.transform.in_hi = 8.0f;
    glitch.transform.smoothing_ms = 60.0f;
    glitch.destination.target = "fx.a.glitch.amount";
    mapping_.add(glitch);

    Mapping lfo;
    lfo.name = "LFO 1 (2 temps) -> kaléidoscope";
    lfo.source.kind = SourceKind::Modulator;
    lfo.source.index = 0;
    lfo.transform.out_hi = 360.0f;
    lfo.destination.target = "fx.a.kaleidoscope.rotation";
    mapping_.add(lfo);

    Mapping envelope;
    envelope.name = "enveloppe scratch -> bloom";
    envelope.source.kind = SourceKind::Modulator;
    envelope.source.index = 1;
    envelope.destination.target = "fx.a.bloom.amount";
    mapping_.add(envelope);
}

void Engine::anchor_now(double now_s) {
    anchor_.set(a_.timecode.state().position_s, a_.played.position_s, now_s,
                a_.timecode.jump_count());
}

void Engine::set_window_budget(std::uint64_t bytes) {
    window_budget_ = std::max<std::uint64_t>(bytes, 16ull << 20);
    for (Deck* deck : {&a_, &b_, &overlay_}) {
        deck->window_budget_bytes = window_budget_;
        if (deck->clip.frame_count == 0) continue;
        WindowConfig window_config;
        window_config.budget_bytes = window_budget_;
        deck->window.configure(deck->clip.frame_count,
                               block_bytes_per_frame(deck->clip.width, deck->clip.height,
                                                     deck->clip.format),
                               window_config);
    }
}

std::vector<std::string> Engine::bind() {
    xfader_ = surface_.find("xfader");
    fader_a_ = surface_.find("ch1.fader");
    fader_b_ = surface_.find("ch2.fader");
    std::vector<std::string> unknown = mapping_.resolve(surface_);

    resolved_dest_.assign(mapping_.size(), Dest::None);
    trigger_high_.assign(mapping_.size(), false);
    for (std::size_t i = 0; i < mapping_.size(); ++i) {
        const Destination& destination = mapping_.at(i).destination;
        if (destination.kind != DestinationKind::Local) continue;  // OSC goes out, not in
        resolved_dest_[i] = resolve_destination(destination.target);
        if (resolved_dest_[i] == Dest::None) unknown.push_back(destination.target);
    }
    return unknown;
}

std::vector<std::string> Engine::install_mappings(std::vector<Mapping> mappings) {
    mapping_.clear();
    for (Mapping& m : mappings) mapping_.add(std::move(m));
    return bind();
}

EngineRequests Engine::take_requests() {
    const EngineRequests out = requests_;
    requests_ = EngineRequests{};
    return out;
}

// One mapping's value, applied. Levels are set every step; triggers are
// edges, derived here: a pad mapped to a cue jumps on the press, and holding
// it does not jump sixty times a second.
void Engine::dispatch(std::size_t index, Dest dest, float value, DeckCommands& a,
                      DeckCommands& b) {
    if (is_trigger(dest)) {
        const bool high = value > 0.5f;
        const bool fired = high && !trigger_high_[index];
        trigger_high_[index] = high;
        if (!fired) return;
    }
    const auto unit01 = [value] { return std::clamp(value, 0.0f, 1.0f); };
    const auto set_param = [&](std::size_t slot, int field) {
        if (slot >= rack_.size()) return;
        EffectUnit& unit = rack_.at(slot);
        EffectParams& params = unit.link ? unit.shared : unit.video_override;
        switch (field) {
            case 0: params.mix = unit01(); break;
            case 1: params.amount = unit01(); break;
            case 2: params.time = unit01(); break;
            case 3: params.feedback = unit01(); break;
            case 4: params.depth = unit01(); break;
            case 5: params.tone = unit01(); break;
            case 6: unit.enabled = value > 0.5f; break;
            default: break;
        }
    };
    const auto by_type = [&](EffectType type, auto&& apply) {
        for (std::size_t slot = 0; slot < rack_.size(); ++slot) {
            if (rack_.at(slot).type == type && rack_.at(slot).enabled) apply(rack_.at(slot));
        }
    };

    const int cue = cue_of(dest);
    const int fx_slot = fx_slot_of(dest);
    if (cue >= 0) {
        DeckCommands& commands =
            static_cast<std::uint16_t>(dest) <= static_cast<std::uint16_t>(Dest::DeckACue8) ? a : b;
        commands.cue_jump = true;
        commands.cue_index = cue;
        return;
    }
    if (fx_slot >= 0) {
        const int field = static_cast<int>(static_cast<std::uint16_t>(dest) -
                                           static_cast<std::uint16_t>(Dest::Fx1Mix)) % 7;
        set_param(static_cast<std::size_t>(fx_slot), field);
        return;
    }

    switch (dest) {
        case Dest::MixTransition: mix_.transition = transition_from_unit(value); break;
        case Dest::MixXfaderCurve: mix_.xfader = curve_from_unit(value); break;
        case Dest::MixFaderCurve:
            mix_.channel = curve_from_unit(value);
            mix_.channel_b = mix_.channel;
            break;
        case Dest::MixFaderACurve: mix_.channel = curve_from_unit(value); break;
        case Dest::MixFaderBCurve: mix_.channel_b = curve_from_unit(value); break;
        case Dest::MixFaderAReverse: mix_.channel_reverse = value > 0.5f; break;
        case Dest::MixFaderBReverse: mix_.channel_b_reverse = value > 0.5f; break;
        case Dest::MixXfaderReverse: mix_.xfader_reverse = value > 0.5f; break;
        case Dest::MixFaderReverse:
            mix_.channel_reverse = value > 0.5f;
            mix_.channel_b_reverse = mix_.channel_reverse;
            break;

        case Dest::DeckAYaw: view_a_.yaw_deg = static_cast<double>(value); break;
        case Dest::DeckAPitch: view_a_.pitch_deg = static_cast<double>(value); break;
        case Dest::DeckAFov: view_a_.fov_deg = std::clamp(static_cast<double>(value), 20.0, 170.0); break;
        case Dest::DeckAZoom: view_a_.planet_zoom = std::clamp(static_cast<double>(value), 0.1, 4.0); break;

        case Dest::DeckALoopIn: a.loop_in = true; break;
        case Dest::DeckALoopOut: a.loop_out = true; break;
        case Dest::DeckALoopExit: a.loop_exit = true; break;
        case Dest::DeckBLoopIn: b.loop_in = true; break;
        case Dest::DeckBLoopOut: b.loop_out = true; break;
        case Dest::DeckBLoopExit: b.loop_exit = true; break;
        case Dest::DeckASlip:
            if (value > 0.5f) a.slip_on = true; else a.slip_off = true;
            break;
        case Dest::DeckBSlip:
            if (value > 0.5f) b.slip_on = true; else b.slip_off = true;
            break;
        case Dest::DeckALoadNext: requests_.load_next_a = true; break;
        case Dest::DeckBLoadNext: requests_.load_next_b = true; break;
        case Dest::OverlayLoadNext: requests_.load_next_overlay = true; break;

        case Dest::FxFilterAmount:
            by_type(EffectType::LowPass, [&](EffectUnit& unit) { unit.shared.amount = unit01(); });
            break;
        case Dest::FxKaleidoscopeRotation:
            // The LFO runs 0..360 for the OSC mirror; the rack wants 0..1.
            by_type(EffectType::Kaleidoscope, [&](EffectUnit& unit) {
                unit.video_override.amount = std::clamp(value / 360.0f, 0.0f, 1.0f);
            });
            break;
        case Dest::FxGlitchAmount:
            by_type(EffectType::Datamosh, [&](EffectUnit& unit) { unit.shared.amount = unit01(); });
            break;
        case Dest::FxBloomAmount:
            by_type(EffectType::Bloom, [&](EffectUnit& unit) { unit.shared.amount = unit01(); });
            break;

        case Dest::OverlayOpacity: overlay_layer_.opacity = unit01(); break;
        case Dest::OverlayEnabled: overlay_layer_.enabled = value > 0.5f; break;

        case Dest::LibraryNext: requests_.library_step += 1; break;
        case Dest::LibraryPrev: requests_.library_step -= 1; break;
        case Dest::LibraryLoadA: requests_.library_load_a = true; break;
        case Dest::LibraryLoadB: requests_.library_load_b = true; break;
        case Dest::LibraryLoadOverlay: requests_.library_load_overlay = true; break;

        case Dest::None:
        default: break;
    }
}

void Engine::step(const EngineFrame& frame) {
    // Commands first, against the position the transport is still holding from the
    // last frame: a loop set on the beat must start where the platter was when the
    // button was pressed, not where it has reached by the time we look. The
    // front end's commands and last step's mapped ones (a pad on a cue) merge:
    // both are buttons, and a button is a button whoever pressed it.
    const auto run_commands = [](Deck& deck, const DeckCommands& commands) {
        if (commands.slip_on) deck.transport.set_slip(true);
        if (commands.loop_in) {
            deck.transport.loop_in(deck.transport.position_s());
            deck.transport.loop_out(deck.transport.position_s() + commands.loop_seconds);
        }
        if (commands.loop_out) deck.transport.loop_out(deck.transport.position_s());
        if (commands.loop_exit) deck.transport.exit_loop();
        if (commands.slip_off) deck.transport.set_slip(false);
        // Set before jump: a pad pressed with shift held sets, and the same
        // frame must not then jump to what it just set -- but a set and a
        // jump in one frame from two different buttons still both apply.
        if (commands.cue_set) {
            deck.transport.set_cue(commands.cue_index, deck.transport.position_s());
        }
        if (commands.cue_clear) deck.transport.clear_cue(commands.cue_index);
        if (commands.cue_jump && !commands.cue_set) {
            deck.transport.jump_to_cue(commands.cue_index);
        }
        if (commands.auto_loop_beats > 0.0) deck.transport.auto_loop(commands.auto_loop_beats);
        if (commands.beat_jump_beats != 0.0) deck.transport.beat_jump(commands.beat_jump_beats);
    };
    run_commands(a_, frame.commands_a);
    run_commands(b_, frame.commands_b);

    a_.advance(frame.deck_a);
    b_.advance(frame.deck_b);
    overlay_.advance_free(frame.time_s);

    motion_a_ = a_.timecode.state().link == LinkState::Lost
                    ? apply_link_loss(policy_, motion_a_, frame.dt_s)
                    : measured(a_);
    motion_b_ = b_.timecode.state().link == LinkState::Lost
                    ? apply_link_loss(policy_, motion_b_, frame.dt_s)
                    : measured(b_);

    // An untouched crossfader sits in the middle, where a battle mixer's does;
    // untouched channel faders are open. See value_or().
    const float crossfader = value_or(surface_, xfader_, 0.5f);
    const float fader_a = value_or(surface_, fader_a_, 1.0f);
    const float fader_b = value_or(surface_, fader_b_, 1.0f);
    xfade_position_ = mix_.xfader_reverse ? 1.0f - crossfader : crossfader;
    cuts_.update(frame.time_s, xfade_position_);
    weights_ = mix_weights(crossfader, fader_a, fader_b, mix_);
    stack_ = stack_weights(crossfader, fader_a, fader_b, mix_, overlay_layer_);

    // The LFO's phase comes from the played position, so it follows the platter --
    // backwards included -- instead of marching on regardless. That is why this
    // happens after advance() and not before it.
    // The played position, not the transport's: everything downstream must see
    // the position the picture is actually at, after the clip-boundary fold.
    const double beat_position = a_.played.position_s * (bpm_ / 60.0);
    modulators_.set_envelope_input(1, motion_a_.scratch_rate / 12.0f);
    modulators_.update(frame.time_s, beat_position, frame.dt_s);

    EngineInputs inputs;
    inputs.deck[0].velocity = motion_a_.velocity;
    inputs.deck[0].scratch_rate = motion_a_.scratch_rate;
    inputs.deck[0].acceleration = motion_a_.acceleration;
    inputs.deck[0].confidence = a_.timecode.state().confidence;
    inputs.deck[0].position_s = static_cast<float>(a_.played.position_s);
    inputs.deck[1].velocity = motion_b_.velocity;
    inputs.deck[1].scratch_rate = motion_b_.scratch_rate;
    inputs.deck[1].confidence = b_.timecode.state().confidence;
    inputs.deck[1].position_s = static_cast<float>(b_.played.position_s);
    if (a_.gestures.backspin()) inputs.gesture_bits |= kGestureBackspinA;
    inputs.modulators = modulators_.values().data();
    inputs.modulator_count = modulators_.size();

    // The reactive bands. A set where no audio ever arrives leaves them decaying
    // to zero rather than frozen: a disconnected input must not pin the video on
    // whatever the last sound happened to be, which reads as a hung machine.
    audio_silent_s_ += frame.dt_s;
    if (audio_silent_s_ > 0.05) {
        spectrum_.advance_silent(audio_silent_s_);
        audio_silent_s_ = 0.0;
    }
    inputs.bands = spectrum_.bands().data();
    inputs.band_count = spectrum_.band_count();
    mapping_.evaluate(surface_, inputs, frame.dt_s);

    // Every mapping reaches its destination through one table (core/
    // destinations): a knob, an LFO or a gesture drives an effect, the gaze,
    // a cue or the mixer's curve by the same door. A GHOST source -- an
    // absolute pot never touched since launch -- leaves its destination
    // exactly where it is: its real position is unknown, and acting on the
    // fabricated zero would wrench the view to dead centre at startup. This
    // is the same rule the interface draws those pots dashed for. Transport
    // edges land in next step's commands, so a mapped cue and a clicked cue
    // arrive by the same path at the same moment.
    DeckCommands mapped_a, mapped_b;
    for (std::size_t i = 0; i < mapping_.size(); ++i) {
        if (!mapping_.active(i) || i >= resolved_dest_.size()) continue;
        const Mapping& m = mapping_.at(i);
        if (m.source.kind == SourceKind::Control) {
            const ControlIndex control = surface_.find(m.source.control_id);
            if (control == kNoControl || !surface_.at(control).known) continue;
        }
        dispatch(i, resolved_dest_[i], mapping_.value(i), mapped_a, mapped_b);
    }
    // Loop and cue edges from mappings apply now, on the position this step
    // produced, rather than waiting a frame: a pad is a button, and a button
    // that answers one frame late is felt.
    if (mapped_a.any()) run_commands(a_, mapped_a);
    if (mapped_b.any()) run_commands(b_, mapped_b);
}

void Engine::analyse_audio(const float* samples, std::size_t count) {
    if (samples == nullptr || count == 0) return;
    // Any window this produces resets the silence timer, so the decay in step()
    // only runs once audio has genuinely stopped arriving -- not merely because
    // a block was shorter than one window.
    spectrum_.push(samples, count);
    audio_silent_s_ = 0.0;
}

SchemaPacket Engine::schema() const {
    SchemaPacket packet;
    packet.entries = schema_from(surface_);
    packet.schema_hash = svj::schema_hash(packet.entries);
    return packet;
}

StatePacket Engine::packet(std::uint64_t t_us, std::uint32_t hash) const {
    StatePacket packet;
    packet.t_us = t_us;
    packet.schema_hash = hash;

    const auto fill = [](DeckWire& wire, const Deck& deck, const DeckMotion& motion) {
        wire.pos_s = static_cast<float>(deck.transport.position_s());
        wire.velocity = motion.velocity;
        wire.acceleration = motion.acceleration;
        wire.scratch_rate = motion.scratch_rate;
        wire.confidence = deck.timecode.state().confidence;
    };
    fill(packet.deck_a, a_, motion_a_);
    fill(packet.deck_b, b_, motion_b_);

    if (a_.gestures.scratching()) packet.gesture_bits |= kGestureScratchingA;
    if (a_.gestures.backspin()) packet.gesture_bits |= kGestureBackspinA;
    if (a_.timecode.state().link == LinkState::Lost) packet.gesture_bits |= kGestureHoldingA;
    if (b_.gestures.scratching()) packet.gesture_bits |= kGestureScratchingB;

    packet.values.reserve(surface_.size());
    packet.known.reserve(surface_.size());
    for (std::size_t i = 0; i < surface_.size(); ++i) {
        const Control& c = surface_.at(static_cast<ControlIndex>(i));
        packet.values.push_back(c.value);
        packet.known.push_back(c.known);
    }
    return packet;
}

}  // namespace svj
