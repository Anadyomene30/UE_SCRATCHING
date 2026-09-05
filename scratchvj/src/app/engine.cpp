#include "app/engine.h"

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

    TimecodeConfig config;
    config.profile = profile;
    config.mode = TransportMode::Relative;
    timecode = TimecodeTracker(config);

    transport.configure(clip.duration_s(), BeatGrid{bpm, 0.0});
    clock.configure(clip.duration_s(), bpm);

    WindowConfig window_config;
    window_config.budget_bytes = 256ull << 20;  // 256 MiB, enough to see it slide
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

void Engine::configure(double bpm) {
    bpm_ = bpm;

    a_.configure("DECK A  tokyo_nightdrive_360", 240.0, 3840, 1920, BlockFormat::BC1,
                 SignalProfile::Wireless, bpm);
    b_.configure("DECK B  grain_loop_04", 12.0, 1920, 1080, BlockFormat::BC7,
                 SignalProfile::Wireless, bpm);
    // The third layer. It has no platter by construction, so it is the first
    // thing in the engine that exercises core/playback: tempo-locked, one pass
    // per four beats, bouncing rather than cutting back to its head -- an eight
    // second texture has a visible seam on a wrap and none on a bounce.
    overlay_.configure("INCRUST  city_grid_mask", 8.0, 1280, 720, BlockFormat::BC7,
                       SignalProfile::Wireless, bpm);
    overlay_.clock.set_source(DeckSource::TempoLocked, 0.0);
    overlay_.clock.set_beats_per_cycle(4.0, 0.0);
    overlay_.clock.set_mode(ClipPlayMode::PingPong);
    // A stray hand on a platter must never capture the layer holding the mask.
    overlay_.clock.set_takeover(TakeoverMode::Ignore);
    overlay_layer_.enabled = true;
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

    a_.transport.set_cue(0, 0.0, 1);
    a_.transport.set_cue(1, 30.0, 2);
    a_.transport.set_cue(2, 96.0, 3);

    anchor_.set(0.0, 0.0, 0.0, 0);

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
    rack_.load(2, EffectType::SlitScan);
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

    mapping_.add(
        control_mapping("crossfader -> transition A/B", "xfader", "mix.transition", 0.0f, 1.0f));

    Mapping glitch;
    glitch.name = "deck A vitesse -> glitch";
    glitch.source.kind = SourceKind::DeckVelocity;
    glitch.transform.in_lo = -8.0f;
    glitch.transform.in_hi = 8.0f;
    glitch.transform.smoothing_ms = 60.0f;
    glitch.destination.target = "fx.a.glitch.amount";
    mapping_.add(glitch);

    Mapping shake;
    shake.name = "deck A scratch/s -> OSC /ue/shake";
    shake.source.kind = SourceKind::DeckScratchRate;
    shake.transform.in_hi = 12.0f;
    shake.destination.kind = DestinationKind::Osc;
    shake.destination.target = "/ue/shake";
    mapping_.add(shake);

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

    Mapping whip;
    whip.name = "backspin A -> OSC /ue/whippan";
    whip.source.kind = SourceKind::Gesture;
    whip.source.gesture_bit = kGestureBackspinA;
    whip.destination.kind = DestinationKind::Osc;
    whip.destination.target = "/ue/whippan";
    mapping_.add(whip);
}

std::vector<std::string> Engine::bind() {
    xfader_ = surface_.find("xfader");
    fader_a_ = surface_.find("ch1.fader");
    fader_b_ = surface_.find("ch2.fader");
    return mapping_.resolve(surface_);
}

void Engine::step(const EngineFrame& frame) {
    // Commands first, against the position the transport is still holding from the
    // last frame: a loop set on the beat must start where the platter was when the
    // button was pressed, not where it has reached by the time we look.
    const DeckCommands& commands = frame.commands_a;
    if (commands.slip_on) a_.transport.set_slip(true);
    if (commands.loop_in) {
        a_.transport.loop_in(a_.transport.position_s());
        a_.transport.loop_out(a_.transport.position_s() + commands.loop_seconds);
    }
    if (commands.loop_exit) a_.transport.exit_loop();
    if (commands.slip_off) a_.transport.set_slip(false);
    if (commands.cue_jump) a_.transport.jump_to_cue(commands.cue_index);

    a_.advance(frame.deck_a);
    b_.advance(frame.deck_b);
    overlay_.advance_free(frame.time_s);

    motion_a_ = a_.timecode.state().link == LinkState::Lost
                    ? apply_link_loss(policy_, motion_a_, frame.dt_s)
                    : measured(a_);
    motion_b_ = b_.timecode.state().link == LinkState::Lost
                    ? apply_link_loss(policy_, motion_b_, frame.dt_s)
                    : measured(b_);

    const float crossfader = value_of(surface_, xfader_);
    cuts_.update(frame.time_s, crossfader);
    weights_ = mix_weights(crossfader, value_of(surface_, fader_a_),
                           value_of(surface_, fader_b_), FaderCurve::Sharp);
    stack_ = stack_weights(crossfader, value_of(surface_, fader_a_),
                           value_of(surface_, fader_b_), FaderCurve::Sharp,
                           overlay_layer_);

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
    mapping_.evaluate(surface_, inputs, frame.dt_s);
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
