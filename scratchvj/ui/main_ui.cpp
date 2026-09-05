// scratchvj — the window.
//
// SDL3 for the window and the input, Dear ImGui for the interface, exactly as
// the roadmap picked them. What runs inside is the SAME Engine and the SAME
// Simulation the console demo runs: this front end adds a window, and nothing
// else. When there are real turntables the simulation is swapped for a MIDI and
// timecode source and not one line below this file changes.
//
// No bgfx yet, deliberately. bgfx is for drawing decoded video frames and there
// are none until the FFmpeg analysis pass exists; ImGui's SDL_Renderer backend
// carries the interface until then. The place it will attach is the filmstrip
// and the two deck panels -- everything else here is widgets, which bgfx would
// not change.
#include <SDL3/SDL.h>
// Provides the WinMain the Windows GUI subsystem links against, so the app opens
// as a window with no console behind it. Header-only in SDL3, and it has to be
// included in the translation unit that defines main().
#include <SDL3/SDL_main.h>

#include <chrono>
#include <cstdio>

#include "app/engine.h"
#include "app/simulation.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "panels.h"

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

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

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
    const float scale = SDL_GetWindowDisplayScale(window);
    const float dpi = scale > 0.0f ? scale : 1.0f;

    ImGuiIO& io = ImGui::GetIO();
    char* base = SDL_GetBasePath() != nullptr ? SDL_strdup(SDL_GetBasePath()) : nullptr;
    if (base != nullptr) {
        char path[1024];
        std::snprintf(path, sizeof(path), "%sfonts/Archivo-Variable.ttf", base);
        svj::ui::g_fonts.sans = io.Fonts->AddFontFromFileTTF(path, 17.0f * dpi);
        svj::ui::g_fonts.small = io.Fonts->AddFontFromFileTTF(path, 13.0f * dpi);
        std::snprintf(path, sizeof(path), "%sfonts/DMMono-Regular.ttf", base);
        svj::ui::g_fonts.mono = io.Fonts->AddFontFromFileTTF(path, 17.0f * dpi);
        SDL_free(base);
    }
    if (svj::ui::g_fonts.sans == nullptr) {
        // No font files next to the binary: fall back, and scale the bitmap face
        // up so it is at least legible.
        ImGui::GetStyle().FontScaleMain = 1.55f * dpi;
    } else {
        io.FontDefault = svj::ui::g_fonts.sans;
    }
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    Engine engine;
    engine.configure(kBpm);
    Simulation simulation;
    simulation.configure(engine.surface());
    for (const std::string& id : engine.bind()) {
        std::fprintf(stderr, "mapping refers to an unknown control: %s\n", id.c_str());
    }

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

        EngineFrame frame;
        frame.time_s = wall_s;
        frame.dt_s = dt;
        frame.now_us = now_us;
        frame.deck_a = simulation.deck_a();
        frame.deck_b = simulation.deck_b();
        frame.commands_a = to_commands(simulation.events());
        engine.step(frame);

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        svj::ui::Frame view;
        view.elapsed_s = t;
        view.phase = simulation.phase();
        svj::ui::draw(engine, view);

        ImGui::Render();
        SDL_SetRenderDrawColorFloat(renderer, 0.078f, 0.078f, 0.071f, 1.0f);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
