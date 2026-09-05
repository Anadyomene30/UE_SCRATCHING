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
#include "app/engine.h"
#include "app/simulation.h"
#include "core/effect.h"
#include "core/layout.h"
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

// The script's events, as the engine's command struct. The demo is a front end
// like any other: it says what the buttons did, and the engine decides what that
// means for the transport.
DeckCommands to_commands(const SimEvent& events) {
    DeckCommands commands;
    commands.loop_in = events.loop_in;
    commands.loop_exit = events.loop_exit;
    commands.slip_on = events.slip_on;
    commands.slip_off = events.slip_off;
    commands.cue_jump = events.cue_jump;
    commands.cue_index = events.cue_index;
    return commands;
}

int run_demo(int argc, char** argv) {
    const double seconds = std::stod(option(argc, argv, "--seconds", "24"));
    const double fps = std::stod(option(argc, argv, "--fps", "30"));
    const bool ansi = !flag(argc, argv, "--plain");
    const std::string record_path = option(argc, argv, "--record", "");

    Engine engine;
    engine.configure(kDemoBpm);

    Simulation simulation;
    simulation.configure(engine.surface());
    for (const std::string& id : engine.bind()) {
        std::cerr << "mapping refers to an unknown control: " << id << "\n";
    }

    const SchemaPacket schema = engine.schema();

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
        simulation.step(t, engine.surface(), now_us);

        EngineFrame frame;
        frame.time_s = t;
        frame.dt_s = static_cast<float>(dt);
        frame.now_us = now_us;
        frame.deck_a = simulation.deck_a();
        frame.deck_b = simulation.deck_b();
        frame.commands_a = to_commands(simulation.events());
        engine.step(frame);

        if (!record_path.empty()) {
            if (!take.write_state(engine.packet(now_us, schema.schema_hash), error)) {
                std::cerr << error << "\n";
                return 1;
            }
        }

        Deck& a = engine.deck_a();
        Deck& b = engine.deck_b();

        DashboardView view;
        view.elapsed_s = t;
        view.phase = simulation.phase();
        view.bpm = engine.bpm();
        view.anchor = &engine.anchor();
        view.jump_count = a.timecode.jump_count();
        view.weights = engine.weights();
        view.cuts = &engine.cuts();
        view.rack = &engine.rack();
        view.modulators = &engine.modulators();

        Deck& ov = engine.overlay();
        DeckView va{a.name, &a.timecode.state(), &a.gestures, &a.transport,
                    &a.window, &a.clip, &a.played};
        DeckView vb{b.name, &b.timecode.state(), &b.gestures, &b.transport,
                    &b.window, &b.clip, &b.played};
        DeckView vo{ov.name, &ov.timecode.state(), &ov.gestures, &ov.transport,
                    &ov.window, &ov.clip, &ov.played};
        view.overlay = &engine.overlay_layer();
        view.overlay_deck = &vo;
        view.overlay_gain = engine.stack().overlay;

        std::cout << render_dashboard(view, va, vb, engine.surface(), engine.mapping(), ansi)
                  << std::flush;

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
