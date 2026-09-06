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
#include <cstdio>
#include <filesystem>
#include <vector>

#include "app/engine.h"
#include "app/simulation.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <bgfx/bgfx.h>

#include "imgui_impl_bgfx.h"
#include "gpu_compose.h"
#include "gpu_view360.h"
#include "media.h"
#include "netout.h"
#include "panels.h"
#include "share.h"

namespace {

using namespace svj;

constexpr double kBpm = 124.0;
constexpr double kScriptSeconds = 24.0;

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

int main(int, char**) {
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
    bgfx::setViewClear(2, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x141412ff, 1.0f, 0);

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
    // View order is the pipeline order: 0 reprojects deck A's 360 view, 1
    // composites the program offscreen, 2 draws the interface (which samples
    // both), 3 blits the program out for readback.
    if (!svj::ui::ImGuiBgfx_Init(2)) {
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
    svj::ui::ProgramGpu gpu;
    svj::ui::View360Gpu view360a;

    // The Spout sender is opened unconditionally: receivers that are not
    // listening cost nothing, and an output that must be switched on before it
    // can be discovered never gets discovered.
    svj::ui::ProgramShare share;
    share.open("scratchvj");

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

    if (media_a.ready() || media_b.ready()) {
        const std::uint32_t pw = media_a.ready() ? media_a.width() : media_b.width();
        const std::uint32_t ph = media_a.ready() ? media_a.height() : media_b.height();
        if (!gpu.init(pw, ph, 1, 3)) {
            std::fprintf(stderr, "compositeur GPU: init a echoue\n");
        }
    }
    if (media_a.ready() && media_a.header().is_equirect()) {
        // Deck A gets the 360 view pass: downstream of it -- compositor and
        // interface alike -- a 360 deck behaves as an ordinary flat deck
        // showing the projected view.
        if (view360a.init(1024, 576, 0)) {
            engine.view_a().aspect = 1024.0 / 576.0;
        } else {
            std::fprintf(stderr, "passe 360: init a echoue\n");
        }
    }

    // Controls the hand has claimed from the demo script; applied after every
    // simulation step so the hand always wins, exactly as MIDI will.
    svj::ui::HandState hand;
    // The view state that outlives a frame: which layout is up. Everything else
    // in Frame is refilled each pass.
    svj::ui::Frame view;

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
        previous_s = t;

        const auto now_us = static_cast<std::uint64_t>(wall_s * 1e6);
        simulation.step(t, engine.surface(), now_us);
        for (const auto& owned : hand.owned) {
            engine.surface().set(owned.first, owned.second, now_us);
        }

        EngineFrame frame;
        frame.time_s = wall_s;
        frame.dt_s = dt;
        frame.now_us = now_us;
        frame.deck_a = simulation.deck_a();
        frame.deck_b = simulation.deck_b();
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
        if (view360a.ready()) {
            view360a.render(media_a.texture_index(), engine.view_a());
            view.tex_a = view360a.imgui_texture();
        }
        view.tex_b = media_b.frame_at(engine.deck_b().played.position_s);
        media_overlay.frame_at(engine.overlay().played.position_s);

        // The program: what actually leaves the machine, composited on the GPU
        // by the shader gpu_check holds to core/compose. The interface previews
        // the render target itself; Spout receives the readback a couple of
        // frames later, which a video feed cannot see.
        if (gpu.ready()) {
            gpu.render(view360a.ready() ? view360a.texture_index()
                                        : media_a.texture_index(),
                       media_b.texture_index(),
                       media_overlay.texture_index(), engine.stack().a,
                       engine.stack().b, engine.stack().overlay,
                       static_cast<int>(engine.overlay_layer().blend));
            view.tex_program = gpu.imgui_texture();
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
        bgfx::setViewRect(2, 0, 0, static_cast<std::uint16_t>(pixel_w),
                          static_cast<std::uint16_t>(pixel_h));
        bgfx::touch(2);  // the clear runs even on a frame with nothing else
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
