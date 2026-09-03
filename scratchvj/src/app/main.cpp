// scratchvj — command line entry point.
//
// No hardware is required for `demo`: a scripted performance drives the whole
// engine so the behaviour can be watched, recorded and replayed. That is what
// makes the rest of the project developable before the turntables are plugged in.
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "app/dashboard.h"
#include "app/simulation.h"
#include "core/effect.h"
#include "core/layout.h"
#include "core/mixer.h"
#include "core/modulator.h"
#include "core/protocol.h"
#include "core/take.h"

namespace {

using namespace svj;

constexpr double kDemoBpm = 124.0;

void usage() {
    std::cout <<
        "scratchvj — a scratchable video DJ instrument\n"
        "\n"
        "  scratchvj demo [--seconds N] [--fps N] [--plain] [--record FILE]\n"
        "        Runs a scripted performance through the whole engine and draws it.\n"
        "        No turntables needed. --record writes a .scratchtake fixture.\n"
        "\n"
        "  scratchvj play FILE [--fps N] [--plain]\n"
        "        Replays a recorded take.\n"
        "\n"
        "  scratchvj info FILE.svcache\n"
        "        Prints what an analysed clip contains.\n"
        "\n"
        "  scratchvj effects\n"
        "        Prints the effect battery and how each audio effect maps to video.\n"
        "\n"
        "  scratchvj layout\n"
        "        Lists the controls --midi-learn will ask you to sweep.\n"
        "\n"
        "  scratchvj version\n";
}

bool flag(int argc, char** argv, const char* name) {
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], name) == 0) return true;
    }
    return false;
}

std::string option(int argc, char** argv, const char* name, const std::string& fallback) {
    for (int i = 2; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], name) == 0) return argv[i + 1];
    }
    return fallback;
}

// Everything a deck needs, wired together the way the real application will.
struct Deck {
    TimecodeTracker timecode;
    GestureTracker gestures;
    Transport transport;
    FrameWindow window;
    CacheHeader clip;
    std::string name;

    void configure(std::string label, double duration_s, std::uint32_t width,
                   std::uint32_t height, BlockFormat format, SignalProfile profile) {
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

        transport.configure(clip.duration_s(), BeatGrid{kDemoBpm, 0.0});

        WindowConfig window_config;
        window_config.budget_bytes = 256ull << 20;  // 256 MiB, to show it sliding
        window.configure(clip.frame_count,
                         block_bytes_per_frame(width, height, format), window_config);
    }

    double advance(const DecoderSample& sample) {
        const TimecodeState& state = timecode.submit(sample);
        gestures.update(sample.time_s, state.velocity, state.confidence);
        const double played = transport.map(state.position_s);
        window.update(clip.frame_at(played), state.velocity);
        return played;
    }
};

void build_mappings(MappingEngine& mapping) {
    const auto control = [](std::string name, std::string id, std::string target,
                            float lo, float hi) {
        Mapping m;
        m.name = std::move(name);
        m.source.kind = SourceKind::Control;
        m.source.control_id = std::move(id);
        m.transform.out_lo = lo;
        m.transform.out_hi = hi;
        m.destination.target = std::move(target);
        return m;
    };

    mapping.add(control("ch1.eq.hi -> deck A yaw 360", "ch1.eq.hi", "deck.a.yaw", -180.0f, 180.0f));
    mapping.add(control("ch1.eq.mid -> deck A pitch 360", "ch1.eq.mid", "deck.a.pitch", -90.0f, 90.0f));

    Mapping filter = control("ch1.filter -> cutoff + flou", "ch1.filter", "fx.a.filter", 0.0f, 1.0f);
    filter.transform.deadzone = 0.08f;
    mapping.add(filter);

    mapping.add(control("crossfader -> transition A/B", "xfader", "mix.transition", 0.0f, 1.0f));

    Mapping glitch;
    glitch.name = "deck A vitesse -> glitch";
    glitch.source.kind = SourceKind::DeckVelocity;
    glitch.transform.in_lo = -8.0f;
    glitch.transform.in_hi = 8.0f;
    glitch.transform.smoothing_ms = 60.0f;
    glitch.destination.target = "fx.a.glitch.amount";
    mapping.add(glitch);

    Mapping shake;
    shake.name = "deck A scratch/s -> OSC /ue/shake";
    shake.source.kind = SourceKind::DeckScratchRate;
    shake.transform.in_hi = 12.0f;
    shake.destination.kind = DestinationKind::Osc;
    shake.destination.target = "/ue/shake";
    mapping.add(shake);

    Mapping lfo;
    lfo.name = "LFO 1 (2 temps) -> kaléidoscope";
    lfo.source.kind = SourceKind::Modulator;
    lfo.source.index = 0;
    lfo.transform.out_hi = 360.0f;
    lfo.destination.target = "fx.a.kaleidoscope.rotation";
    mapping.add(lfo);

    Mapping envelope;
    envelope.name = "enveloppe scratch -> bloom";
    envelope.source.kind = SourceKind::Modulator;
    envelope.source.index = 1;
    envelope.destination.target = "fx.a.bloom.amount";
    mapping.add(envelope);

    Mapping whip;
    whip.name = "backspin A -> OSC /ue/whippan";
    whip.source.kind = SourceKind::Gesture;
    whip.source.gesture_bit = kGestureBackspinA;
    whip.destination.kind = DestinationKind::Osc;
    whip.destination.target = "/ue/whippan";
    mapping.add(whip);
}

StatePacket to_packet(std::uint64_t t_us, std::uint32_t schema, const Deck& a, const Deck& b,
                      const Surface& surface) {
    StatePacket packet;
    packet.t_us = t_us;
    packet.schema_hash = schema;

    const auto fill = [](DeckWire& wire, const Deck& deck) {
        wire.pos_s = static_cast<float>(deck.transport.position_s());
        wire.velocity = deck.timecode.state().velocity;
        wire.acceleration = deck.gestures.acceleration();
        wire.scratch_rate = deck.gestures.scratch_rate();
        wire.confidence = deck.timecode.state().confidence;
    };
    fill(packet.deck_a, a);
    fill(packet.deck_b, b);

    if (a.gestures.scratching()) packet.gesture_bits |= kGestureScratchingA;
    if (a.gestures.backspin()) packet.gesture_bits |= kGestureBackspinA;
    if (a.timecode.state().link == LinkState::Lost) packet.gesture_bits |= kGestureHoldingA;
    if (b.gestures.scratching()) packet.gesture_bits |= kGestureScratchingB;

    packet.values.reserve(surface.size());
    packet.known.reserve(surface.size());
    for (std::size_t i = 0; i < surface.size(); ++i) {
        const Control& c = surface.at(static_cast<ControlIndex>(i));
        packet.values.push_back(c.value);
        packet.known.push_back(c.known);
    }
    return packet;
}

int run_demo(int argc, char** argv) {
    const double seconds = std::stod(option(argc, argv, "--seconds", "24"));
    const double fps = std::stod(option(argc, argv, "--fps", "30"));
    const bool ansi = !flag(argc, argv, "--plain");
    const std::string record_path = option(argc, argv, "--record", "");

    Surface surface;
    Simulation simulation;
    simulation.configure(surface);

    MappingEngine mapping;
    build_mappings(mapping);
    const auto unresolved = mapping.resolve(surface);
    for (const std::string& id : unresolved) {
        std::cerr << "mapping refers to an unknown control: " << id << "\n";
    }

    Deck a, b;
    a.configure("DECK A  tokyo_nightdrive_360", 240.0, 3840, 1920, BlockFormat::BC1,
                SignalProfile::Wireless);
    b.configure("DECK B  grain_loop_04", 12.0, 1920, 1080, BlockFormat::BC7,
                SignalProfile::Wireless);
    a.transport.set_cue(0, 0.0, 1);
    a.transport.set_cue(1, 30.0, 2);
    a.transport.set_cue(2, 96.0, 3);

    Anchor anchor;
    anchor.set(0.0, 0.0, 0.0, 0);

    // A tempo-synced LFO and an envelope following how hard the platter is being
    // worked. Both reach the mapping engine as plain numbers.
    ModulatorBank modulators;
    Lfo sweep;
    sweep.shape = LfoShape::Triangle;
    sweep.beats = 2.0;
    modulators.add(sweep);
    Envelope follower;
    follower.attack_ms = 20.0f;
    follower.release_ms = 400.0f;
    modulators.add(follower);

    EffectRack rack(3);
    rack.load(0, EffectType::Delay);
    rack.at(0).sync.tempo = true;
    rack.at(0).sync.beats = 0.5;
    rack.at(0).shared.mix = 0.62f;
    rack.at(0).shared.feedback = 0.55f;
    rack.load(1, EffectType::LowPass);
    rack.at(1).shared.mix = 0.41f;
    // Unlinked on purpose, to show the one state where the two domains diverge.
    rack.load(2, EffectType::SlitScan);
    rack.at(2).shared.mix = 0.77f;
    rack.at(2).unlink();
    rack.at(2).audio_override.mix = 0.0f;

    CutDetector cuts;

    SchemaPacket schema;
    schema.entries = schema_from(surface);
    schema.schema_hash = svj::schema_hash(schema.entries);

    TakeWriter take;
    std::string error;
    if (!record_path.empty()) {
        if (!take.open(record_path, error) || !take.write_schema(schema, error)) {
            std::cerr << error << "\n";
            return 1;
        }
    }

    const double dt = 1.0 / fps;
    const auto started = std::chrono::steady_clock::now();

    for (double t = 0.0; t < seconds; t += dt) {
        const auto now_us = static_cast<std::uint64_t>(t * 1e6);
        simulation.step(t, surface, now_us);

        const SimEvent& events = simulation.events();
        if (events.slip_on) a.transport.set_slip(true);
        if (events.loop_in) {
            a.transport.loop_in(a.transport.position_s());
            a.transport.loop_out(a.transport.position_s() + 2.0);
        }
        if (events.loop_exit) a.transport.exit_loop();
        if (events.slip_off) a.transport.set_slip(false);
        if (events.cue_jump) a.transport.jump_to_cue(events.cue_index);

        a.advance(simulation.deck_a());
        b.advance(simulation.deck_b());

        const float crossfader = surface.at(surface.find("xfader")).value;
        cuts.update(t, crossfader);
        const MixWeights weights =
            mix_weights(crossfader, surface.at(surface.find("ch1.fader")).value,
                        surface.at(surface.find("ch2.fader")).value, FaderCurve::Sharp);

        // The LFO's phase comes from the played position, so it follows the
        // platter -- backwards included -- instead of marching on regardless.
        const double beat_position = a.transport.position_s() * (kDemoBpm / 60.0);
        modulators.set_envelope_input(1, a.gestures.scratch_rate() / 12.0f);
        modulators.update(t, beat_position, static_cast<float>(dt));

        EngineInputs inputs;
        inputs.deck[0].velocity = a.timecode.state().velocity;
        inputs.deck[0].scratch_rate = a.gestures.scratch_rate();
        inputs.deck[0].confidence = a.timecode.state().confidence;
        inputs.deck[0].position_s = static_cast<float>(a.transport.position_s());
        inputs.deck[1].velocity = b.timecode.state().velocity;
        if (a.gestures.backspin()) inputs.gesture_bits |= kGestureBackspinA;
        inputs.modulators = modulators.values().data();
        inputs.modulator_count = modulators.size();
        mapping.evaluate(surface, inputs, static_cast<float>(dt));

        if (!record_path.empty()) {
            if (!take.write_state(to_packet(now_us, schema.schema_hash, a, b, surface), error)) {
                std::cerr << error << "\n";
                return 1;
            }
        }

        DashboardView view;
        view.elapsed_s = t;
        view.phase = simulation.phase();
        view.bpm = kDemoBpm;
        view.anchor = &anchor;
        view.jump_count = a.timecode.jump_count();
        view.weights = weights;
        view.cuts = &cuts;
        view.rack = &rack;
        view.modulators = &modulators;

        DeckView va{a.name, &a.timecode.state(), &a.gestures, &a.transport, &a.window, &a.clip};
        DeckView vb{b.name, &b.timecode.state(), &b.gestures, &b.transport, &b.window, &b.clip};
        std::cout << render_dashboard(view, va, vb, surface, mapping, ansi) << std::flush;

        const auto target = started + std::chrono::duration<double>(t + dt);
        std::this_thread::sleep_until(target);
    }

    if (!record_path.empty()) {
        if (!take.close(error)) {
            std::cerr << error << "\n";
            return 1;
        }
        std::cout << "\nprise enregistrée : " << record_path << " (" << take.records_written()
                  << " enregistrements)\n";
    }
    return 0;
}

int run_play(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return 2;
    }
    const double fps = std::stod(option(argc, argv, "--fps", "30"));
    const bool ansi = !flag(argc, argv, "--plain");

    TakeReader reader;
    std::string error;
    if (!reader.open(argv[2], error)) {
        std::cerr << error << "\n";
        return 1;
    }

    SchemaPacket schema;
    StatePacket state;
    PacketKind kind{};
    Surface surface;
    bool have_schema = false;
    std::uint64_t first_us = 0;
    bool first = true;
    const auto started = std::chrono::steady_clock::now();

    while (reader.next(kind, state, schema, error)) {
        if (kind == PacketKind::Schema) {
            surface = Surface{};
            for (const SchemaEntry& e : schema.entries) surface.declare(e.id, e.kind);
            have_schema = true;
            continue;
        }
        if (!have_schema) continue;

        for (std::size_t i = 0; i < state.values.size() && i < surface.size(); ++i) {
            if (state.known[i]) {
                surface.set(static_cast<ControlIndex>(i), state.values[i], state.t_us);
            }
        }

        if (first) {
            first_us = state.t_us;
            first = false;
        }
        const double elapsed = static_cast<double>(state.t_us - first_us) / 1e6;

        DashboardView view;
        view.elapsed_s = elapsed;
        view.phase = "relecture de la prise";
        view.bpm = 0.0;

        TimecodeState ta;
        ta.position_s = state.deck_a.pos_s;
        ta.velocity = state.deck_a.velocity;
        ta.confidence = state.deck_a.confidence;
        ta.link = (state.gesture_bits & kGestureHoldingA) ? LinkState::Lost : LinkState::Ok;

        TimecodeState tb;
        tb.position_s = state.deck_b.pos_s;
        tb.velocity = state.deck_b.velocity;
        tb.confidence = state.deck_b.confidence;
        tb.link = LinkState::Ok;

        DeckView va{"DECK A", &ta, nullptr, nullptr, nullptr, nullptr};
        DeckView vb{"DECK B", &tb, nullptr, nullptr, nullptr, nullptr};
        MappingEngine empty;
        std::cout << render_dashboard(view, va, vb, surface, empty, ansi) << std::flush;

        std::this_thread::sleep_until(started + std::chrono::duration<double>(elapsed));
        (void)fps;
    }
    if (!error.empty()) {
        std::cerr << error << "\n";
        return 1;
    }
    return 0;
}

int run_info(int argc, char** argv) {
    if (argc < 3) {
        usage();
        return 2;
    }
    CacheReader reader;
    std::string error;
    if (!reader.open(argv[2], error)) {
        std::cerr << error << "\n";
        return 1;
    }
    const CacheHeader& h = reader.header();
    std::cout << argv[2] << "\n"
              << "  " << h.width << "x" << h.height << "  " << h.frame_count << " frames  "
              << (static_cast<double>(h.fps_num) / h.fps_den) << " fps  "
              << h.duration_s() << " s\n"
              << "  format      " << (h.format == BlockFormat::BC1   ? "BC1"
                                      : h.format == BlockFormat::BC3 ? "BC3"
                                                                     : "BC7")
              << "\n"
              << "  alpha       " << (h.has_alpha() ? "oui" : "non") << "\n"
              << "  360         " << (h.is_equirect() ? "équirectangulaire" : "non") << "\n"
              << "  par frame   " << reader.frame_bytes() << " octets\n"
              << "  métadonnées " << reader.metadata().size() << " octets\n";
    return 0;
}

int run_effects() {
    std::size_t paired = 0, video_only = 0;
    for (const EffectDescriptor& d : effect_catalogue()) {
        if (d.relation == Correspondence::VideoOnly) ++video_only;
        else ++paired;
    }
    std::cout << effect_catalogue().size() << " effets — " << paired << " paires audio/vidéo, "
              << video_only << " vidéo seuls\n\n";

    std::cout << "PAIRES\n";
    for (const EffectDescriptor& d : effect_catalogue()) {
        if (d.relation == Correspondence::VideoOnly) continue;
        std::cout << "  " << d.id
                  << std::string(d.id && std::strlen(d.id) < 14 ? 14 - std::strlen(d.id) : 1, ' ')
                  << (d.relation == Correspondence::Identical ? "[identique]" : "[analogue] ")
                  << "\n      audio  " << d.audio << "\n      vidéo  " << d.video << "\n";
    }

    std::cout << "\nVIDÉO SEULS\n";
    for (const EffectDescriptor& d : effect_catalogue()) {
        if (d.relation != Correspondence::VideoOnly) continue;
        std::cout << "  " << d.id
                  << std::string(d.id && std::strlen(d.id) < 14 ? 14 - std::strlen(d.id) : 1, ' ')
                  << d.video << "\n";
    }
    return 0;
}

int run_layout() {
    const auto targets = default_rig_layout();
    std::cout << targets.size() << " contrôles à apprendre\n\n";
    for (const LearnTarget& t : targets) {
        const char* kind = "?";
        switch (t.kind) {
            case ControlKind::Fader: kind = "fader"; break;
            case ControlKind::Knob: kind = "potard"; break;
            case ControlKind::Button: kind = "bouton"; break;
            case ControlKind::Encoder: kind = "encodeur"; break;
            case ControlKind::Pad: kind = "pad"; break;
        }
        std::cout << "  " << t.id << std::string(t.id.size() < 26 ? 26 - t.id.size() : 1, ' ')
                  << kind << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    const std::string command = argv[1];
    if (command == "demo") return run_demo(argc, argv);
    if (command == "play") return run_play(argc, argv);
    if (command == "info") return run_info(argc, argv);
    if (command == "effects") return run_effects();
    if (command == "layout") return run_layout();
    if (command == "version") {
        std::cout << "scratchvj 0.1.0 — socle, sans audio ni vidéo réels\n";
        return 0;
    }
    usage();
    return 2;
}
