// scratchvj — the window.
//
// SDL3 for the window and the input, Dear ImGui for the interface, exactly as
// the roadmap picked them. What runs inside is the SAME Engine and the SAME
// Simulation the console demo runs: this front end adds a window, and nothing
// else. When there are real turntables the simulation is swapped for a MIDI and
// timecode source and not one line below this file changes.
//
// bgfx underneath, as the roadmap chose: the interface is rendered by bgfx and
// the deck frames reach the GPU as the BC1 blocks the cache stores, with no CPU
// expansion on the display path. Single-threaded on purpose (renderFrame before
// init): the render loop IS the app loop, and a second thread would buy jitter
// before it bought speed.
#include <SDL3/SDL.h>
// Provides the WinMain the Windows GUI subsystem links against, so the app opens
// as a window with no console behind it. Header-only in SDL3, and it has to be
// included in the translation unit that defines main().
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#if defined(_MSC_VER)
#include <share.h>
#endif
#include <cstdio>
#include <filesystem>
#include <vector>

#include "app/engine.h"
#include "app/simulation.h"
#include "core/quadrature.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <bgfx/bgfx.h>

#include "imgui_impl_bgfx.h"
#include "gpu_compose.h"
#include "gpu_effects.h"
#include "gpu_taps.h"
#include "gpu_view360.h"
#include "audio_in.h"
#include "live_in.h"
#include "media.h"
#include "netout.h"
#include "panels.h"
#include "share.h"

namespace {

using namespace svj;

constexpr double kBpm = 124.0;
constexpr double kScriptSeconds = 24.0;
// The name this application publishes its program under, and the one a live
// receiver has to recognise as its own to warn about a feedback loop.
constexpr const char* kShareName = "scratchvj";

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

}  // namespace

int main(int argc, char** argv) {
    // `--live`: start with deck A on the real platter rather than the script.
    // A performer booting for a set wants this without a click, and a test
    // driving the window from outside wants it without a synthetic mouse --
    // which proved too fragile to trust for anything that matters.
    bool start_live = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--live") start_live = true;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("scratchvj", 1440, 1040,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int pixel_w = 0, pixel_h = 0;
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    // renderFrame() before init() keeps bgfx single-threaded: the render loop
    // IS the app loop, and a second thread would buy jitter before speed.
    bgfx::renderFrame();
    bgfx::Init init;
    init.swapChain.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                                nullptr);
    init.swapChain.width = static_cast<std::uint32_t>(pixel_w);
    init.swapChain.height = static_cast<std::uint32_t>(pixel_h);
    init.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init)) {
        std::fprintf(stderr, "bgfx::init a echoue\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // The mockup's ground colour, painted by the clear rather than by a quad.
    // View 1 is the backbuffer; view 0 belongs to the program compositor.
    bgfx::setViewClear(8, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x141412ff, 1.0f, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;  // no layout file next to the binary
    svj::ui::apply_style();

    // The mockup's faces, not ImGui's default bitmap one. This is not decoration:
    // the whole interface is numbers read at arm's length while both hands are on
    // the platters, so a real text face at a real size, and a monospace with
    // tabular figures for anything that has to line up in a column, is the
    // difference between glanceable and unreadable. Both are OFL and vendored in
    // ui/fonts. A missing file degrades to the default face rather than failing.
    // Pixel density, NOT the display scale. The SDL window is addressed in
    // pixels; on a 150% Windows desktop SDL_GetWindowDisplayScale answers 1.5
    // and every face comes out anywhere from too big to enormous -- which is
    // exactly how the first screenshot of this interface looked. The density
    // (drawable pixels per logical unit) is 1.0 on Windows and >1 only on
    // genuinely high-DPI drawables (macOS Retina), which is the factor the font
    // rasteriser actually needs.
    const float density = SDL_GetWindowPixelDensity(window);
    const float dpi = density > 0.0f ? density : 1.0f;

    ImGuiIO& io = ImGui::GetIO();
    char* base = SDL_GetBasePath() != nullptr ? SDL_strdup(SDL_GetBasePath()) : nullptr;
    if (base != nullptr) {
        char path[1024];
        // Sizes read off the mockup at its native 1440 width: body 15, labels
        // 11.5, numbers 15. The interface is dense on purpose; big type was the
        // single largest reason the first build did not look like the design.
        std::snprintf(path, sizeof(path), "%sfonts/Archivo-Variable.ttf", base);
        svj::ui::g_fonts.sans = io.Fonts->AddFontFromFileTTF(path, 15.0f * dpi);
        svj::ui::g_fonts.small = io.Fonts->AddFontFromFileTTF(path, 11.5f * dpi);
        std::snprintf(path, sizeof(path), "%sfonts/DMMono-Regular.ttf", base);
        svj::ui::g_fonts.mono = io.Fonts->AddFontFromFileTTF(path, 15.0f * dpi);
        SDL_free(base);
    }
    if (svj::ui::g_fonts.sans == nullptr) {
        // No font files next to the binary: fall back, and scale the bitmap face
        // up so it is at least legible.
        ImGui::GetStyle().FontScaleMain = 1.55f * dpi;
    } else {
        io.FontDefault = svj::ui::g_fonts.sans;
    }
    ImGui_ImplSDL3_InitForOther(window);
    // View order IS pipeline order, and bgfx runs views by id: 0 and 1 are the
    // decks' multi-tap passes (trails and slit scan, which read the CLIP and so
    // must come before anything that leaves it behind), 2 reprojects deck A's
    // 360 view, 3 composites the program, 4..7 are the single-frame effect
    // rack's passes, 8 draws the interface, and 9 blits the result for
    // readback. Getting this order wrong is not a crash -- it is Spout quietly
    // carrying last frame's picture.
    if (!svj::ui::ImGuiBgfx_Init(8)) {
        std::fprintf(stderr, "backend ImGui bgfx: echec\n");
        return 1;
    }

    Engine engine;
    engine.configure(kBpm);
    Simulation simulation;
    simulation.configure(engine.surface());
    for (const std::string& id : engine.bind()) {
        std::fprintf(stderr, "mapping refers to an unknown control: %s\n", id.c_str());
    }

    // Analysed clips, if any exist. `clips/*.svcache` next to the working
    // directory is where `scratchvj analyze` leaves them; deck assignment is by
    // the clip's own nature -- equirect footage goes to deck A (the 360 deck of
    // the demo), the shortest clip becomes the overlay texture, the rest is B.
    // With no caches at all, the fabricated demo decks stay and the wells say
    // honestly why they are empty.
    svj::ui::DeckMedia media_a, media_b, media_overlay;
    // Opened only when the performer asks for it: a Spout receiver held open
    // permanently would keep a DirectX device alive for a feature nobody is
    // using, and would connect to whatever sender happened to appear.
    svj::ui::LiveInput live_overlay;

    // The real platter: the Phase's carrier off an audio input, read by
    // core/quadrature. Opened only when asked, like the Spout receiver, and
    // ALWAYS in shared mode -- Serato may be on the same interface.
    //
    // The endpoint and channel pair are the ones measured on this desk
    // (docs/cablage.md): the MOTU's 5/6, 1000 Hz. Configuration, not code, is
    // where these belong eventually; until a settings file carries them they
    // are stated here in one place rather than guessed in several.
    svj::ui::AudioInput platter_in;
    QuadratureTracker platter;
    std::vector<float> platter_pcm;
    constexpr const char* kPlatterEndpoint = "MOTU";
    constexpr unsigned kPlatterFirstChannel = 4;  // 0-based: channels 5/6
    svj::ui::ProgramGpu gpu;
    svj::ui::View360Gpu view360a;
    svj::ui::EffectsGpu effects;
    svj::ui::TapsGpu taps_a, taps_b;

    // The Spout sender is opened unconditionally: receivers that are not
    // listening cost nothing, and an output that must be switched on before it
    // can be discovered never gets discovered.
    svj::ui::ProgramShare share;
    share.open(kShareName);

    // The control stream, on localhost by default: Unreal, TouchDesigner or the
    // net_check tool listen on the same machine first. Opened unconditionally
    // for the same reason the Spout sender is.
    svj::ui::ControlStream control;
    control.open("127.0.0.1", svj::ui::kDefaultControlPort);
    const SchemaPacket wire_schema = engine.schema();
    double last_schema_sent_s = -10.0;
    {
        std::vector<std::filesystem::path> caches;
        std::error_code missing;
        for (const auto& entry : std::filesystem::directory_iterator("clips", missing)) {
            if (entry.path().extension() == ".svcache") caches.push_back(entry.path());
        }
        std::sort(caches.begin(), caches.end());

        // Real caches replace the fabricated library outright. Mixing the demo's
        // invented entries with clips that actually exist would make the browser
        // half-true, which is worse than either whole.
        if (!caches.empty()) {
            engine.library() = Library{};
            engine.queue() = Queue{};
            engine.library().create_crate("Tous les clips");
        }

        const auto try_load = [&](svj::ui::DeckMedia& media, Deck& deck,
                                  const std::filesystem::path& path) {
            std::string error;
            if (!media.open(path.string(), error)) {
                std::fprintf(stderr, "%s: %s\n", path.string().c_str(), error.c_str());
                return false;
            }
            deck.load(media.header(), path.stem().string(), kBpm);
            ClipEntry entry;
            entry.path = path.string();
            entry.name = path.stem().string();  // "clip.mp4", not "clip.mp4.svcache"
            entry.duration_s = media.header().duration_s();
            entry.width = media.header().width;
            entry.height = media.header().height;
            entry.equirect = media.header().is_equirect();
            entry.bpm = kBpm;
            const ClipId id = engine.library().add(entry);
            engine.library().set_state(id, AnalysisState::Ready, 1.0f);
            engine.library().add_to_crate(0, id);
            return true;
        };

        // Pick by role rather than by order on disk.
        std::stable_sort(caches.begin(), caches.end(),
                         [](const auto& a, const auto& b) {
                             return std::filesystem::file_size(a) >
                                    std::filesystem::file_size(b);
                         });
        std::vector<std::filesystem::path> remaining;
        for (const auto& path : caches) {
            if (!media_a.ready()) {
                if (try_load(media_a, engine.deck_a(), path)) continue;
            } else if (!media_b.ready()) {
                if (try_load(media_b, engine.deck_b(), path)) continue;
            } else if (!media_overlay.ready()) {
                if (try_load(media_overlay, engine.overlay(), path)) continue;
            }
            remaining.push_back(path);
        }
        (void)remaining;
    }

    // Controls the hand has claimed from the demo script; applied after every
    // simulation step so the hand always wins, exactly as MIDI will.
    svj::ui::HandState hand;
    // The view state that outlives a frame: which layout is up. Everything else
    // in Frame is refilled each pass.
    svj::ui::Frame view;
    view.deck_a_live = start_live;

    // The GPU passes are sized from the clips, so loading a different clip has
    // to rebuild them. Doing it in one place means a load cannot leave half the
    // pipeline addressing the old resolution -- which does not crash, it just
    // shows a corner of the new clip stretched over the old target.
    const auto rebuild_passes = [&]() {
        if (media_a.ready() || media_b.ready()) {
            const std::uint32_t pw = media_a.ready() ? media_a.width() : media_b.width();
            const std::uint32_t ph = media_a.ready() ? media_a.height() : media_b.height();
            if (!gpu.ready() || gpu.width() != pw || gpu.height() != ph) {
                if (!gpu.init(pw, ph, 3, 9)) {
                    std::fprintf(stderr, "compositeur GPU: init a echoue\n");
                }
                if (gpu.ready() && !effects.init(pw, ph, 4)) {
                    std::fprintf(stderr, "effets GPU: init a echoue\n");
                }
            }
        }
        // The taps passes run at each deck's own clip resolution: they read the
        // clip, so they belong to the clip's grid rather than to the program's.
        // init() destroys first, so the guard is only there to avoid
        // recompiling a shader whose target has not moved.
        const auto fit_taps = [](svj::ui::TapsGpu& pass, const svj::ui::DeckMedia& media,
                                 std::uint16_t view_id, const char* name) {
            if (!media.ready()) return;
            if (pass.ready() && pass.width() == media.width() &&
                pass.height() == media.height()) {
                return;
            }
            if (!pass.init(media.width(), media.height(), view_id)) {
                std::fprintf(stderr, "passe taps %s: init a echoue\n", name);
            }
        };
        fit_taps(taps_a, media_a, 0, "A");
        fit_taps(taps_b, media_b, 1, "B");

        // The 360 pass exists only while deck A actually holds spherical
        // footage. Loading a flat clip onto it must take the pass away, or the
        // interface would keep showing a reprojection of a picture that is not
        // a sphere.
        if (media_a.ready() && media_a.header().is_equirect()) {
            if (!view360a.ready()) {
                if (view360a.init(1024, 576, 2)) {
                    engine.view_a().aspect = 1024.0 / 576.0;
                } else {
                    std::fprintf(stderr, "passe 360: init a echoue\n");
                }
            }
        } else {
            view360a.destroy();
        }
    };
    rebuild_passes();

    const auto started = std::chrono::steady_clock::now();
    double previous_s = 0.0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED &&
                event.window.windowID == SDL_GetWindowID(window)) {
                SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
                bgfx::SwapChain resized;
                resized.width = static_cast<std::uint32_t>(pixel_w);
                resized.height = static_cast<std::uint32_t>(pixel_h);
                bgfx::reset(BGFX_RESET_VSYNC, &resized);
            }
        }

        // The script loops, so the window can be left open and watched. Wall
        // clock drives it rather than a frame counter: the engine is a function
        // of time, and feeding it a counter would make the picture depend on how
        // fast this machine happens to redraw.
        const double wall_s =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        const double t = std::fmod(wall_s, kScriptSeconds);
        const float dt = static_cast<float>(t >= previous_s ? t - previous_s : t);
        const double span_from = previous_s;
        previous_s = t;

        // A clip the panel asked for LAST frame, served now -- before anything
        // builds a draw list. Loading closes the deck's cache, and an ImGui
        // draw list already holds that texture as an ImTextureID; destroying it
        // between draw() and Render() submits a dead handle. It survived the
        // first time only because bgfx handed the same recycled index back, so
        // the bug hid until a load also freed the 360 pass and shifted which
        // index came back. The panel names the clip; the frame boundary applies
        // it.
        if (view.load_clip != kNoClip && view.load_target != DeckTarget::None) {
            const ClipEntry& entry = engine.library().at(view.load_clip);
            const bool to_a = view.load_target == DeckTarget::A;
            svj::ui::DeckMedia& media = to_a ? media_a : media_b;
            Deck& deck = to_a ? engine.deck_a() : engine.deck_b();

            std::string error;
            if (media.open(entry.path, error)) {
                deck.load(media.header(), entry.name, kBpm);
                rebuild_passes();
            } else {
                std::fprintf(stderr, "%s: %s\n", entry.path.c_str(), error.c_str());
            }
        }
        view.load_clip = kNoClip;

        const auto now_us = static_cast<std::uint64_t>(wall_s * 1e6);

        // The sound the script is notionally playing, into the reactive bands.
        // Synthetic until an audio device exists -- but the analyser, the
        // mapping and everything downstream are the real ones, so what is on
        // screen is the actual behaviour rather than a mock of it. Skipped on
        // the frame the script wraps, where the span would run backwards.
        if (t > span_from) {
            static std::vector<float> audio(4096);
            const std::size_t written =
                simulation.audio(span_from, t, kBpm, 48000.0, audio.data(), audio.size());
            engine.analyse_audio(audio.data(), written);
        }

        simulation.step(t, engine.surface(), now_us);
        for (const auto& owned : hand.owned) {
            engine.surface().set(owned.first, owned.second, now_us);
        }

        // The platter's source, served at the frame boundary. Opening an audio
        // device is I/O, so the panel only asks and this is where it happens.
        if (view.deck_a_live && !platter_in.ready()) {
            if (platter_in.open(kPlatterEndpoint, kPlatterFirstChannel)) {
                QuadratureConfig config;
                config.carrier_hz = 1000.0;  // measured: docs/cablage.md
                config.sample_rate = platter_in.sample_rate();
                if (!platter.configure(config)) {
                    std::fprintf(stderr, "plateau live: configuration refusee\n");
                    platter_in.close();
                    view.deck_a_live = false;
                }
            } else {
                std::fprintf(stderr, "plateau live: entree \"%s\" introuvable\n",
                             kPlatterEndpoint);
                view.deck_a_live = false;
            }
        } else if (!view.deck_a_live && platter_in.ready()) {
            platter_in.close();
        }

        EngineFrame frame;
        frame.time_s = wall_s;
        frame.dt_s = dt;
        frame.now_us = now_us;
        frame.deck_a = simulation.deck_a();
        frame.deck_b = simulation.deck_b();

        if (platter_in.ready()) {
            // Everything captured since last frame, through the tracker, and
            // the result replaces the script's deck A. The tracker's output IS
            // a DecoderSample, so nothing downstream knows the difference --
            // that is the seam the whole engine was built on.
            platter_in.drain(platter_pcm);
            if (!platter_pcm.empty()) {
                frame.deck_a = platter.submit(platter_pcm.data(), platter_pcm.size() / 2,
                                              wall_s);
            } else {
                // Nothing arrived this frame: hold the last reading rather than
                // feeding the engine a fresh "no signal". A 60 Hz loop can
                // legitimately outrun a 2 ms capture thread for one frame.
                frame.deck_a.time_s = wall_s;
                frame.deck_a.position_s = platter.locked() ? platter.position_s() : -1.0;
                frame.deck_a.pitch = platter.velocity();
                frame.deck_a.signal_level = platter.level();
                frame.deck_a.locked = platter.locked();
            }
        }
        view.platter_connected = platter_in.ready();
        // The short form: "In 1-24 (MOTU Pro Audio)" does not fit on the row,
        // and the part before the bracket is what identifies the input anyway.
        {
            std::string name = platter_in.endpoint_name();
            const std::size_t bracket = name.find(" (");
            if (bracket != std::string::npos) name.resize(bracket);
            if (platter_in.ready()) {
                name += " " + std::to_string(platter_in.first_channel() + 1) + "/" +
                        std::to_string(platter_in.first_channel() + 2);
            }
            view.platter_endpoint = name;
        }
        view.platter_level = platter.level();
        view.platter_locked = platter.locked();
        view.platter_slews = platter.slew_events();

        // Once a second, the platter's truth in a file next to the working
        // directory. A FILE, not stderr: this is a WIN32-subsystem application
        // with no console, so its stderr goes nowhere even when a launcher
        // redirects it -- which is why every earlier "read stderr" came back
        // empty. And a screenshot of the panel is one instant that can coincide
        // with nominal speed by chance; the log is what makes "is it really
        // following the hand" answerable rather than lucky.
        static double last_platter_log_s = -1.0;
        static FILE* platter_log = nullptr;
        if (platter_in.ready() && wall_s - last_platter_log_s >= 1.0) {
            last_platter_log_s = wall_s;
            if (platter_log == nullptr) {
#if defined(_MSC_VER)
                // _SH_DENYNO: fopen_s denies concurrent readers, which makes a
                // log nobody can read while the application runs -- the one
                // time it is wanted.
                platter_log = _fsopen("platter.log", "w", _SH_DENYNO);
#else
                platter_log = std::fopen("platter.log", "w");
#endif
            }
            if (platter_log != nullptr) {
                std::fprintf(platter_log,
                             "plateau %7.2fs  frames=%llu  lock=%d  vel=%+6.3f  "
                             "pos=%9.3f  niveau=%.3f  slew=%u\n",
                             wall_s,
                             static_cast<unsigned long long>(platter_in.frames_captured()),
                             platter.locked() ? 1 : 0,
                             static_cast<double>(platter.velocity()), platter.position_s(),
                             static_cast<double>(platter.level()), platter.slew_events());
                std::fflush(platter_log);
            }
        }
        frame.commands_a = to_commands(simulation.events());
        engine.step(frame);

        // The control stream: state every frame, schema once a second so a
        // client attaching mid-set still learns what the floats mean.
        control.send_state(engine.packet(now_us, wire_schema.schema_hash));
        if (wall_s - last_schema_sent_s >= 1.0) {
            control.send_schema(wire_schema);
            last_schema_sent_s = wall_s;
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        view.elapsed_s = t;
        view.phase = simulation.phase();
        view.hand = &hand;
        view.tex_a = media_a.frame_at(engine.deck_a().played.position_s);
        view.tex_equirect = media_a.imgui_texture();

        // Trails and slit scan first, because they read the CLIP -- once the
        // 360 pass has turned it into a view, the other moments are gone. The
        // rack's first multi-tap slot drives each deck; a deck whose slot is
        // idle, or whose record is not moving, keeps its plain frame.
        const auto tapped = [&](svj::ui::TapsGpu& pass, svj::ui::DeckMedia& media,
                                const Deck& deck) {
            std::uint16_t source = media.texture_index();
            if (!pass.ready()) return source;
            for (std::size_t i = 0; i < engine.rack().size(); ++i) {
                const EffectUnit& unit = engine.rack().at(i);
                if (!unit.video_active() || !is_multi_tap_effect(unit.type)) continue;
                const TapPlan plan =
                    plan_taps(unit, deck.played.position_s, deck.played.velocity,
                              60.0 / engine.bpm(), deck.clip.duration_s(),
                              deck.clock.mode());
                source = pass.render(media.taps_texture(plan), source, unit, plan);
                if (i < 3) view.tap_moments[i] = plan.collapsed ? 1 : plan.count;
                break;  // one multi-tap effect per deck: they all want the layers
            }
            return source;
        };
        const std::uint16_t deck_a_source = tapped(taps_a, media_a, engine.deck_a());
        const std::uint16_t deck_b_source = tapped(taps_b, media_b, engine.deck_b());

        if (view360a.ready()) {
            view360a.render(deck_a_source, engine.view_a());
            view.tex_a = view360a.imgui_texture();
        }
        view.tex_b = media_b.frame_at(engine.deck_b().played.position_s);

        // The overlay's source: a clip, or a live Spout sender. Opened and shut
        // here rather than in the panel, because that is I/O.
        //
        // The live overlay reads the PRESENT -- newest_s() -- and nothing else.
        // The ring's history is there and full, but reaching into it means
        // deciding whether a held position keeps the frame or keeps its distance
        // behind the present, and that decision belongs to a scratchable live
        // DECK (DeckSource::Live), which is not written. Reading only the
        // present is the part that has no such question, so it is the part that
        // ships.
        if (view.overlay_live && !live_overlay.ready()) {
            if (!live_overlay.open("")) {
                std::fprintf(stderr, "entree live: DX11 indisponible\n");
                view.overlay_live = false;
            }
        } else if (!view.overlay_live && live_overlay.ready()) {
            live_overlay.close();
        }

        std::uint16_t overlay_source = media_overlay.texture_index();
        if (live_overlay.ready()) {
            live_overlay.poll(wall_s);
            live_overlay.frame_at(live_overlay.ring().newest_s());
            if (live_overlay.texture_index() != 0xFFFF) {
                overlay_source = live_overlay.texture_index();
            }
        } else {
            media_overlay.frame_at(engine.overlay().played.position_s);
        }
        view.live_connected = live_overlay.connected();
        view.live_sender = live_overlay.sender_name();
        view.live_span_s = static_cast<float>(live_overlay.ring().span_s());
        view.live_is_self = view.live_sender == kShareName;

        // The program: what actually leaves the machine, composited on the GPU
        // by the shader gpu_check holds to core/compose. The interface previews
        // the render target itself; Spout receives the readback a couple of
        // frames later, which a video feed cannot see.
        if (gpu.ready()) {
            gpu.render(view360a.ready() ? view360a.texture_index() : deck_a_source,
                       deck_b_source,
                       overlay_source, engine.stack().a,
                       engine.stack().b, engine.stack().overlay,
                       static_cast<int>(engine.overlay_layer().blend));
            // The rack, over the composited program. What leaves the machine
            // is what the effects made of it, so Spout and the preview see the
            // same thing the audience does.
            const std::uint16_t shown = effects.render(gpu.texture_index(), engine.rack());
            view.tex_program =
                reinterpret_cast<void*>(static_cast<std::uint64_t>(shown) + 1);
            view.effect_passes = effects.passes();
            gpu.queue_readback(shown);
            view.program_width = gpu.width();
            view.program_height = gpu.height();
        }
        view.scrubbing = nullptr;
        svj::ui::draw(engine, view);

        // Whatever no widget claimed this frame is not being held. Hand it back
        // to its platter through the Grab takeover, so the picture stays put.
        for (Deck* deck : {&engine.deck_a(), &engine.deck_b()}) {
            if (deck->clock.source() == DeckSource::Hand && view.scrubbing != deck) {
                deck->clock.hand_over_to_timecode(deck->timecode.state().position_s, t);
            }
        }

        ImGui::Render();
        bgfx::setViewRect(8, 0, 0, static_cast<std::uint16_t>(pixel_w),
                          static_cast<std::uint16_t>(pixel_h));
        bgfx::touch(8);  // the clear runs even on a frame with nothing else
        svj::ui::ImGuiBgfx_Render(ImGui::GetDrawData());
        const std::uint32_t frame_number = bgfx::frame();

        // A completed readback of the program, when the GPU has one for us.
        if (const std::uint8_t* pixels = gpu.completed_frame(frame_number)) {
            share.send(pixels, gpu.width(), gpu.height());
        }
    }

    media_a.close();
    media_b.close();
    media_overlay.close();
    taps_a.destroy();
    taps_b.destroy();
    effects.destroy();
    view360a.destroy();
    gpu.destroy();
    svj::ui::ImGuiBgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
