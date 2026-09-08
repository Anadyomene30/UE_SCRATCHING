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
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <vector>

#include "app/analysis_queue.h"
#include "app/engine.h"
#include "app/library_scan.h"
#include "app/simulation.h"
#include "config/library_io.h"
#include "config/mapping_io.h"
#include "config/profile_io.h"
#include "config/settings_io.h"
#include "core/learn.h"
#include "core/profile.h"
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
#include "midi_rig.h"
#include "netout.h"
#include "output_window.h"
#include "thumbnails.h"
#include "core/take.h"
#include <ctime>
#include <filesystem>
#include "core/cachemeta.h"
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

// What the file pickers hand back. SDL may call the picker's callback from
// another thread, so it writes here under a lock and nothing else; the main
// loop drains the box at the frame boundary, where every other library
// request is served. Three lists because a picked file, a picked folder to
// import once and a picked folder to keep watching are three different asks.
struct DialogInbox {
    std::mutex lock;
    std::vector<std::string> files;         // import these
    std::vector<std::string> import_folders; // import what these hold, once
    std::vector<std::string> keep_folders;   // add these to the library folders
};

void SDLCALL on_files_picked(void* userdata, const char* const* filelist, int) {
    if (filelist == nullptr) return;  // cancelled, or an error SDL already logged
    auto* inbox = static_cast<DialogInbox*>(userdata);
    std::lock_guard<std::mutex> guard(inbox->lock);
    for (const char* const* it = filelist; *it != nullptr; ++it) inbox->files.emplace_back(*it);
}

void SDLCALL on_import_folder_picked(void* userdata, const char* const* filelist, int) {
    if (filelist == nullptr) return;
    auto* inbox = static_cast<DialogInbox*>(userdata);
    std::lock_guard<std::mutex> guard(inbox->lock);
    for (const char* const* it = filelist; *it != nullptr; ++it) {
        inbox->import_folders.emplace_back(*it);
    }
}

void SDLCALL on_keep_folder_picked(void* userdata, const char* const* filelist, int) {
    if (filelist == nullptr) return;
    auto* inbox = static_cast<DialogInbox*>(userdata);
    std::lock_guard<std::mutex> guard(inbox->lock);
    for (const char* const* it = filelist; *it != nullptr; ++it) {
        inbox->keep_folders.emplace_back(*it);
    }
}

// Static, because SDL keeps the pointer until the dialog closes.
const SDL_DialogFileFilter kImportFilters[] = {
    {"Vid\xC3\xA9os, images, caches",
     "mp4;m4v;mov;mkv;webm;avi;mxf;ts;mts;m2ts;mpg;mpeg;wmv;flv;ogv;gif;"
     "png;jpg;jpeg;tif;tiff;bmp;exr;dpx;tga;webp;svcache"},
    {"Tous les fichiers", "*"},
};

// A library entry's display name: the file's, with a sequence pattern's
// "%04d" shown as "####" so the list does not read like a printf.
std::string display_name(const std::string& source, const std::string& cache) {
    if (source.empty()) {
        // An orphan cache: "clip.mp4.svcache" -> "clip.mp4".
        return std::filesystem::path(cache).stem().string();
    }
    std::string name = std::filesystem::path(source).filename().string();
    const std::size_t percent = name.find("%0");
    if (percent != std::string::npos) {
        const std::size_t d = name.find('d', percent);
        if (d != std::string::npos) {
            const unsigned digits = static_cast<unsigned>(std::atoi(name.c_str() + percent + 2));
            name.replace(percent, d - percent + 1, std::string(std::max(1u, digits), '#'));
        }
    }
    return name;
}

}  // namespace

int main(int argc, char** argv) {
    // `--live`: start with deck A on the real platter rather than the script.
    // A performer booting for a set wants this without a click, and a test
    // driving the window from outside wants it without a synthetic mouse --
    // which proved too fragile to trust for anything that matters.
    bool start_live = false;
    // `--demo`: the scripted performance and its invented clips. It was the
    // default while there was no hardware and no analysis pass; both exist
    // now, so an instrument that opens full of files the performer does not
    // own -- one of them spherical -- would be lying about what it is.
    bool demo = false;
    // Anything that is not a flag is a file to import, exactly as if it had
    // been dropped on the window: that is what "open with" and dragging onto
    // the executable in Explorer do, and it makes the import path drivable
    // from outside the window.
    std::vector<std::string> opened_with;
    // `--output`: send the program to a screen from the start, named by a
    // fragment of the display's name or by its 1-based number. A rig that is
    // always the same rig should not need a click, and a launcher script that
    // sets the room up is a real way to work.
    std::string output_argument;
    // `--screen`: open on that screen (jouer, bibliotheque, effets, table,
    // sortie, reglages). A launcher can open the library for preparation.
    std::string screen_argument;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--live") {
            start_live = true;
        } else if (argument == "--demo") {
            demo = true;
        } else if (argument == "--output" && i + 1 < argc) {
            output_argument = argv[++i];
        } else if (argument == "--screen" && i + 1 < argc) {
            screen_argument = argv[++i];
        } else if (!argument.empty() && argument[0] != '-') {
            opened_with.push_back(argument);
        }
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
    // rack's passes, 8 draws the interface, 9 blits the result for readback,
    // and 10-11 paint the output screen. Getting this order wrong is not a
    // crash -- it is Spout quietly carrying last frame's picture.
    if (!svj::ui::ImGuiBgfx_Init(8)) {
        std::fprintf(stderr, "backend ImGui bgfx: echec\n");
        return 1;
    }

    Engine engine;
    engine.configure(kBpm, demo ? Engine::DemoContent::Yes : Engine::DemoContent::No);
    Simulation simulation;
    // The simulation still declares the controls it would move, so the surface
    // knows they exist even when nothing is animating them.
    simulation.configure(engine.surface());
    for (const std::string& id : engine.bind()) {
        std::fprintf(stderr, "mapping refers to an unknown control: %s\n", id.c_str());
    }

    // The three layers' pictures. Empty until the performer loads something
    // from the library; with nothing real to show, the fabricated demo decks
    // stay and the wells say honestly why they are empty.
    svj::ui::DeckMedia media_a, media_b, media_overlay;
    // Opened only when the performer asks for it: a Spout receiver held open
    // permanently would keep a DirectX device alive for a feature nobody is
    // using, and would connect to whatever sender happened to appear.
    svj::ui::LiveInput live_overlay;

    // The real platter: the Phase's carrier off an audio input, read by
    // core/quadrature. Opened only when asked, like the Spout receiver, and
    // ALWAYS in shared mode -- Serato may be on the same interface.
    //
    // Which input, which channel pair, which carrier: facts about THIS desk,
    // read from settings.json next to the working directory. A missing file is
    // the measured desk (docs/cablage.md: the MOTU's 5/6, 1000 Hz); a broken
    // one is refused outright rather than half-applied.
    svj::ui::AudioInput platter_in;
    QuadratureTracker platter;
    std::vector<float> platter_pcm;

    // The rig: every MIDI device the settings name, each on its own port.
    // Opened once at startup and re-tried while absent: a MIDI port costs
    // nothing while idle, and an input that has to be switched on before it
    // can be discovered never gets discovered -- the same reasoning the Spout
    // sender is opened under.
    svj::ui::MidiRig rig;
    std::vector<MidiEvent> midi_events;
    MidiLearn learn;
    int learn_device = -1;
    DeskSettings desk;
    std::string startup_error;
    {
        std::string error;
        if (!settings_load("settings.json", desk, error)) startup_error = error;
    }

    // Controller profiles: the built-in ones, plus every file in profiles/
    // next to the working directory and next to the binary.
    std::vector<DeviceProfile> loaded_profiles;
    {
        std::vector<std::string> errors;
        loaded_profiles = profiles_in("profiles", errors);
        if (const char* base_path = SDL_GetBasePath()) {
            for (DeviceProfile& p : profiles_in(std::string(base_path) + "profiles", errors)) {
                if (find_profile(p.name, loaded_profiles) == nullptr) {
                    loaded_profiles.push_back(std::move(p));
                }
            }
        }
        if (!errors.empty()) startup_error = errors.front();
    }

    // The whole rig declared as ghosts before anything is learned or touched,
    // so the table is drawn as it really is: present, and not yet known. A
    // profile that ships bindings (a published protocol) installs them for
    // its device index; the Elite's come from mapping.json below.
    const auto declare_rig = [&]() {
        for (std::size_t i = 0; i < desk.devices.size(); ++i) {
            const DeviceSetting& d = desk.devices[i];
            const DeviceProfile* p = find_profile(d.profile, loaded_profiles);
            if (p == nullptr) continue;
            const DeviceProfile profile = p->name == "rp8000" ? rp8000_profile(d.deck) : *p;
            declare_profile(profile, engine.surface());
            bind_profile(profile, static_cast<std::uint8_t>(i), engine.surface());
        }
    };
    declare_rig();
    // Bindings from a previous session, if any. A missing file is the normal
    // first run. Its mapping rows replace the demo's when it has some.
    SurfaceConfig surface_config;
    {
        std::string error;
        if (config_load("mapping.json", surface_config, error)) {
            config_apply(surface_config, engine.surface());
            if (!surface_config.mappings.empty()) {
                for (const std::string& id : engine.install_mappings(surface_config.mappings)) {
                    startup_error = "mapping.json : inconnu : " + id;
                }
            }
        }
    }
    engine.mix_settings() = desk.mix;
    rig.configure(desk.devices, loaded_profiles);
    svj::ui::ProgramGpu gpu;
    svj::ui::View360Gpu view360a;
    svj::ui::EffectsGpu effects;
    svj::ui::TapsGpu taps_a, taps_b;

    // The Spout sender is opened unconditionally: receivers that are not
    // listening cost nothing, and an output that must be switched on before it
    // can be discovered never gets discovered.
    svj::ui::ProgramShare share;
    share.open(kShareName);

    // The library's pictures, one texture per analysed clip.
    svj::ui::ThumbnailStore thumbnails;

    // The screen the room sees. Opened on request, and reopened at startup on
    // the display the settings name -- by NAME, because display indices are
    // renumbered whenever something is plugged in.
    svj::ui::OutputWindow output;
    std::vector<svj::ui::DisplayInfo> screens;
    double last_displays_s = -10.0;
    bool output_wanted = desk.output_open;
    if (!output_argument.empty()) {
        // The argument wins over what the settings remember, and is resolved
        // against the real list below, once SDL has been asked for it.
        output_wanted = true;
    }

    // The control stream, on localhost by default: Unreal, TouchDesigner or the
    // net_check tool listen on the same machine first. Opened unconditionally
    // for the same reason the Spout sender is.
    svj::ui::ControlStream control;
    control.open(desk.control_host, static_cast<std::uint16_t>(desk.control_port));
    std::string control_endpoint = desk.control_host + ":" + std::to_string(desk.control_port);
    engine.set_window_budget(static_cast<std::uint64_t>(desk.vram_budget_mb) << 20);

    // A take: every packet the wire carries, on disk, for replaying a set or
    // developing without turntables. Opened and closed by the REC button.
    svj::TakeWriter take;
    // The take being replayed, and how its schema maps onto this surface:
    // by control id, so a take from another session's layout still lands.
    svj::TakeReader replay;
    std::vector<ControlIndex> replay_controls;
    double replay_started_s = 0.0;
    std::uint64_t replay_first_us = 0;
    bool replay_have_first = false;
    std::uint32_t replay_read = 0;
    svj::StatePacket replay_state;
    bool replay_state_pending = false;
    const SchemaPacket wire_schema = engine.schema();
    double last_schema_sent_s = -10.0;

    // Controls the hand has claimed from the demo script; applied after every
    // simulation step so the hand always wins, exactly as MIDI will.
    svj::ui::HandState hand;
    // The view state that outlives a frame: the switches JOUER keeps.
    // Everything else in Frame is refilled each pass.
    svj::ui::Frame view;
    view.deck_a_live = start_live;
    if (!screen_argument.empty()) {
        const char* const names[] = {"jouer", "bibliotheque", "effets", "table", "sortie", "reglages"};
        for (int i = 0; i < 6; ++i) {
            if (screen_argument == names[i]) {
                view.screen_request = static_cast<svj::ui::Screen>(i);
                view.screen_requested = true;
            }
        }
    }
    view.script_running = demo;
    view.demo_content = demo;
    view.settings = &desk;
    view.last_error = startup_error;
    view.profile_names.clear();
    for (const DeviceProfile* p : builtin_profiles()) view.profile_names.push_back(p->name);
    for (const DeviceProfile& p : loaded_profiles) view.profile_names.push_back(p.name);

    // --- the library ---------------------------------------------------------
    // What library.json remembers (sources, projections, crates), then what
    // the folders actually hold. Analyses run on their own thread and report
    // back here, at the frame boundary, like every other request.
    AnalysisQueue analysis;
    DialogInbox inbox;
    std::vector<std::string> pending_imports;  // dropped or picked, not yet scanned
    std::vector<AnalysisReport> reports;
    // Clips that arrived by a deliberate gesture -- dropped on the window, or
    // picked in a dialog -- and have not been put anywhere yet. Dropping a
    // video and getting a cache file and nothing on screen is not an import,
    // it is homework; so the first free layer takes it as soon as it is
    // playable. A layer that already holds something is never stolen: that
    // would be an import interrupting a set.
    std::vector<ClipId> awaiting_show;
    double notice_until_s = -100.0;
    pending_imports = opened_with;
    // Whether the library is the performer's or still the demo's fabricated
    // list. The demo list is never written to disk, and the first real clip
    // replaces it whole: mixing invented entries with clips that exist would
    // make the browser half-true, which is worse than either whole.
    bool library_is_real = false;
    const char* kLibraryFile = "library.json";

    // Reads a cache's header into its entry, and marks it playable.
    const auto describe_cache = [&](ClipEntry& entry) {
        CacheReader reader;
        std::string error;
        if (!reader.open(entry.path, error)) return false;
        const CacheHeader& h = reader.header();
        entry.width = h.width;
        entry.height = h.height;
        entry.fps = static_cast<double>(h.fps_num) / static_cast<double>(h.fps_den);
        entry.duration_s = h.duration_s();
        entry.equirect = h.is_equirect();
        entry.has_alpha = h.has_alpha();
        entry.bpm = kBpm;
        entry.state = AnalysisState::Ready;
        entry.progress = 1.0f;
        // The picture, when the cache has one. An older cache has none and
        // shows a blank well; nothing is re-analysed for a thumbnail.
        const CacheMeta meta = decode_meta(reader.metadata());
        entry.thumb_w = meta.thumb_w;
        entry.thumb_h = meta.thumb_h;
        entry.thumbnail = meta.thumb_bc1;
        return true;
    };

    // Folds what a scan found into the library. Returns the entries that are
    // new AND unanalysed, for the caller to decide whether to analyse them.
    const auto merge_items = [&](const std::vector<ScanItem>& items) {
        std::vector<ClipId> fresh;
        if (items.empty()) return fresh;
        if (!library_is_real) {
            engine.library() = Library{};
            engine.queue() = Queue{};
            engine.matrix() = Matrix{};
            library_is_real = true;
        }
        Library& library = engine.library();
        for (const ScanItem& item : items) {
            ClipId id = library.find_by_path(item.cache);
            if (id == kNoClip && !item.source.empty()) id = library.find_by_source(item.source);
            const bool is_new = id == kNoClip;
            if (is_new) {
                ClipEntry entry;
                entry.path = item.cache;
                entry.source_path = item.source;
                entry.name = display_name(item.source, item.cache);
                entry.is_sequence = item.is_sequence;
                entry.is_still = item.is_still;
                id = library.add(entry);
                view.library_dirty = true;
            }
            ClipEntry* entry = library.mutable_at(id);
            if (entry->source_path.empty() && !item.source.empty()) {
                entry->source_path = item.source;
                view.library_dirty = true;
            }
            if (item.cache_exists && !entry->playable()) describe_cache(*entry);
            if (is_new && !entry->playable() && !entry->source_path.empty()) fresh.push_back(id);
        }
        return fresh;
    };

    const auto rescan = [&]() { merge_items(scan_folders(desk.library_folders, desk.cache_dir)); };

    // Hands one clip to the background pass.
    const auto enqueue_analysis = [&](ClipId id) {
        ClipEntry* entry = engine.library().mutable_at(id);
        if (entry == nullptr || entry->source_path.empty()) return;
        if (entry->state == AnalysisState::Queued || entry->state == AnalysisState::Analysing) return;
        AnalyzeOptions options;
        options.input = entry->source_path;
        options.output = entry->path;
        options.max_width = desk.analysis_max_width;
        options.is_still = entry->is_still;
        options.is_sequence = entry->is_sequence;
        if (entry->is_sequence) {
            options.sequence_fps = desk.sequence_fps;
            options.sequence_start = sequence_first_number(entry->source_path);
        }
        engine.library().set_state(id, AnalysisState::Queued);
        analysis.enqueue(id, options);
    };

    {
        LibraryFile remembered;
        std::string error;
        if (!library_load(kLibraryFile, remembered, error)) {
            view.last_error = error;
        } else if (!remembered.clips.empty()) {
            engine.library() = Library{};
            engine.queue() = Queue{};
            library_is_real = true;
            library_restore(remembered, engine.library(), engine.matrix());
            for (std::size_t i = 0; i < engine.library().size(); ++i) {
                ClipEntry* entry = engine.library().mutable_at(static_cast<ClipId>(i));
                describe_cache(*entry);  // playable if its cache is still there
            }
        }
        rescan();
        view.library_dirty = false;  // what was just read back needs no writing
    }

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
        if (media_a.ready() && engine.deck_a().clip.is_equirect()) {
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
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                // Closing the OUTPUT closes the output; only the instrument's
                // own window ends the set.
                if (output.owns(event.window.windowID)) {
                    output_wanted = false;
                } else if (event.window.windowID == SDL_GetWindowID(window)) {
                    running = false;
                }
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE &&
                !ImGui::GetIO().WantTextInput) {
                // One escape gets the picture off the projector; a second ends
                // the set. Quitting straight to a desktop in front of a room
                // is the thing this ordering exists to prevent.
                if (view.full_frame) {
                    view.full_frame = false;
                } else if (output.ready()) {
                    output_wanted = false;
                } else {
                    running = false;
                }
            }
            // Files dropped on the window go through the same door as the
            // picker's: scanned and merged at the frame boundary below.
            if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
                pending_imports.emplace_back(event.drop.data);
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
        // --- the library's requests, all served here ---------------------------
        if (view.import_files_request) {
            SDL_ShowOpenFileDialog(on_files_picked, &inbox, window, kImportFilters,
                                   static_cast<int>(std::size(kImportFilters)), nullptr, true);
            view.import_files_request = false;
        }
        if (view.import_folder_request) {
            SDL_ShowOpenFolderDialog(on_import_folder_picked, &inbox, window, nullptr, false);
            view.import_folder_request = false;
        }
        if (view.add_folder_request) {
            SDL_ShowOpenFolderDialog(on_keep_folder_picked, &inbox, window, nullptr, false);
            view.add_folder_request = false;
        }
        {
            std::lock_guard<std::mutex> guard(inbox.lock);
            pending_imports.insert(pending_imports.end(), inbox.files.begin(), inbox.files.end());
            pending_imports.insert(pending_imports.end(), inbox.import_folders.begin(),
                                   inbox.import_folders.end());
            inbox.files.clear();
            inbox.import_folders.clear();
            for (const std::string& folder : inbox.keep_folders) {
                if (std::find(desk.library_folders.begin(), desk.library_folders.end(), folder) ==
                    desk.library_folders.end()) {
                    desk.library_folders.push_back(folder);
                    view.settings_dirty = true;
                    view.rescan_request = true;
                }
            }
            inbox.keep_folders.clear();
        }
        if (!pending_imports.empty()) {
            // Dropped or picked: the intention is explicit, so what is new is
            // analysed at once, and shown as soon as it can be. A folder scan,
            // by contrast, only lists.
            const std::vector<ScanItem> found = scan_paths(pending_imports, desk.cache_dir);
            for (const ClipId id : merge_items(found)) {
                enqueue_analysis(id);
                awaiting_show.push_back(id);
            }
            // Something already analysed (a cache that was there, or a file
            // imported twice) is playable at once and never produces a report,
            // so it has to be picked up here too.
            for (const ScanItem& item : found) {
                ClipId id = engine.library().find_by_path(item.cache);
                if (id == kNoClip && !item.source.empty()) {
                    id = engine.library().find_by_source(item.source);
                }
                if (id != kNoClip && engine.library().at(id).playable() &&
                    std::find(awaiting_show.begin(), awaiting_show.end(), id) ==
                        awaiting_show.end()) {
                    awaiting_show.push_back(id);
                }
            }
            if (found.empty()) {
                view.last_error = "rien d'ouvrable dans ce qui a ete depose";
            }
            pending_imports.clear();
        }
        if (view.rescan_request) {
            rescan();
            view.rescan_request = false;
        }

        // --- the output screen -------------------------------------------------
        if (wall_s - last_displays_s >= 1.0) {
            last_displays_s = wall_s;
            screens = svj::ui::displays();
            // A projector that has just appeared, and which the settings ask
            // for: take it. This is also how the output comes back at startup.
            if (output_wanted && !output.ready()) {
                // The command line first, by 1-based number or by a fragment
                // of the name; then whatever the settings remember, matched on
                // the full name.
                std::uint32_t wanted = 0;
                if (!output_argument.empty()) {
                    const int index = std::atoi(output_argument.c_str());
                    if (index >= 1 && static_cast<std::size_t>(index) <= screens.size()) {
                        wanted = screens[static_cast<std::size_t>(index - 1)].id;
                    } else {
                        for (const svj::ui::DisplayInfo& screen : screens) {
                            if (screen.name.find(output_argument) == std::string::npos) continue;
                            wanted = screen.id;
                            break;
                        }
                    }
                    if (wanted == 0) {
                        view.output_error = "aucun ecran ne correspond a --output " + output_argument;
                        output_argument.clear();
                        output_wanted = desk.output_open;
                    }
                } else if (!desk.output_display.empty()) {
                    for (const svj::ui::DisplayInfo& screen : screens) {
                        if (screen.name != desk.output_display) continue;
                        wanted = screen.id;
                        break;
                    }
                }
                if (wanted != 0) {
                    // Whether this run came from the command line or from the
                    // settings, opening it does NOT rewrite settings.json: a
                    // flag is an override for one run, and a rig set up by a
                    // launcher script must not quietly become the saved
                    // choice. Only the picker on the SORTIE screen writes.
                    if (output.open(wanted, 10, 11)) {
                        output_argument.clear();
                    } else {
                        view.output_error = output.error();
                        output_argument.clear();
                    }
                }
            }
            // The screen the output is on was unplugged.
            if (output.ready()) {
                bool still_there = false;
                for (const svj::ui::DisplayInfo& screen : screens) {
                    still_there = still_there || screen.id == output.display_id();
                }
                if (!still_there) output.close();
            }
        }
        if (view.open_output_display != 0) {
            if (output.open(view.open_output_display, 10, 11)) {
                view.output_error.clear();
                output_wanted = true;
                for (const svj::ui::DisplayInfo& screen : screens) {
                    if (screen.id != view.open_output_display) continue;
                    desk.output_display = screen.name;
                    break;
                }
                desk.output_open = true;
                view.settings_dirty = true;
            } else {
                view.output_error = output.error();
            }
            view.open_output_display = 0;
        }
        if (view.close_output_request) {
            output_wanted = false;
            view.close_output_request = false;
        }
        if (!output_wanted && output.ready()) {
            output.close();
            if (desk.output_open) {
                desk.output_open = false;
                view.settings_dirty = true;
            }
        }
        view.output_open = output.ready();
        view.displays.clear();
        for (const svj::ui::DisplayInfo& screen : screens) {
            svj::ui::Frame::DisplayView entry;
            entry.id = screen.id;
            entry.name = screen.name;
            entry.width = screen.width;
            entry.height = screen.height;
            entry.refresh_hz = screen.refresh_hz;
            entry.primary = screen.primary;
            entry.is_output = output.ready() && output.display_id() == screen.id;
            view.displays.push_back(std::move(entry));
        }
        if (view.analyse_clip != kNoClip) {
            enqueue_analysis(view.analyse_clip);
            view.analyse_clip = kNoClip;
        }
        if (view.analyse_all_request) {
            for (const ClipId id : engine.library().pending_analysis()) enqueue_analysis(id);
            view.analyse_all_request = false;
        }

        // What the background pass reported since last frame.
        analysis.poll(reports);
        for (const AnalysisReport& report : reports) {
            ClipEntry* entry = engine.library().mutable_at(report.clip);
            if (entry == nullptr) continue;
            if (report.state == AnalysisState::Ready) {
                if (!describe_cache(*entry)) {
                    entry->state = AnalysisState::Failed;
                    view.last_error = entry->name + " : cache illisible";
                } else {
                    view.library_dirty = true;
                }
            } else if (report.state == AnalysisState::Failed) {
                engine.library().set_state(report.clip, AnalysisState::Failed);
                view.last_error = entry->name + " : " + report.error;
            } else {
                engine.library().set_state(report.clip, report.state, report.progress);
            }
        }

        // An imported clip that has become playable goes onto the first free
        // layer, oldest first. Free means the deck holds no cache at all --
        // never a deck a performer has already loaded.
        if (!awaiting_show.empty() && view.load_clip == kNoClip) {
            for (std::size_t i = 0; i < awaiting_show.size(); ++i) {
                const ClipId id = awaiting_show[i];
                const ClipEntry* entry = engine.library().mutable_at(id);
                if (entry == nullptr ||
                    (entry->state != AnalysisState::Ready &&
                     entry->state != AnalysisState::Failed)) {
                    continue;  // still analysing: keep waiting for it
                }
                awaiting_show.erase(awaiting_show.begin() + static_cast<std::ptrdiff_t>(i));
                if (entry->state != AnalysisState::Ready) break;
                const DeckTarget target = !media_a.ready()   ? DeckTarget::A
                                          : !media_b.ready() ? DeckTarget::B
                                                             : DeckTarget::None;
                if (target == DeckTarget::None) {
                    // Both decks are busy: say where it went instead of
                    // interrupting whatever is playing.
                    view.notice = entry->name + " : pret, dans la bibliotheque";
                    notice_until_s = wall_s + 6.0;
                    break;
                }
                view.load_clip = id;
                view.load_target = target;
                view.notice = entry->name + (target == DeckTarget::A ? " sur le deck A"
                                                                     : " sur le deck B");
                notice_until_s = wall_s + 6.0;
                break;
            }
        }
        if (!view.notice.empty() && wall_s > notice_until_s) view.notice.clear();

        // "Next": the head of the queue, through the one path core/library
        // gives it, so a pad and this button can never disagree.
        if (view.load_next_request) {
            view.load_next_request = false;
            const QueueItem next = engine.queue().take_next();
            if (next.clip != kNoClip && engine.library().at(next.clip).playable()) {
                view.load_clip = next.clip;
                view.load_target = default_target(next);
            }
        }

        if (view.load_clip != kNoClip && view.load_target != DeckTarget::None) {
            const ClipEntry& entry = engine.library().at(view.load_clip);
            const bool to_a = view.load_target == DeckTarget::A;
            const bool to_b = view.load_target == DeckTarget::B;
            svj::ui::DeckMedia& media = to_a ? media_a : to_b ? media_b : media_overlay;
            Deck& deck = to_a ? engine.deck_a() : to_b ? engine.deck_b() : engine.overlay();

            std::string error;
            if (media.open(entry.path, error)) {
                // The header says what the analysis pass GUESSED about the
                // shape; the entry says what the performer decided. The deck
                // carries the decision, and every panel and pass asks the deck.
                CacheHeader header = media.header();
                if (effective_equirect(header.is_equirect(), entry.projection)) {
                    header.flags = static_cast<std::uint16_t>(header.flags | kCacheEquirect);
                } else {
                    header.flags = static_cast<std::uint16_t>(header.flags & ~kCacheEquirect);
                }
                deck.load(header, entry.name, kBpm, wall_s, view.load_keep_position);
                (to_a ? view.clip_on_a : to_b ? view.clip_on_b : view.clip_on_overlay) =
                    view.load_clip;
                // A deck is a player: a clip that lands on it PLAYS, unless a
                // platter is driving that deck -- then the record decides, as
                // it always did. Nothing sat frozen at frame zero before this
                // because the platter was there; without one, everything did.
                const bool platter_driving =
                    deck.clock.source() == DeckSource::Timecode &&
                    (view.demo_content || (to_a && view.deck_a_live && view.platter_connected));
                if ((to_a || to_b) && !platter_driving && !view.load_keep_position) {
                    deck.play(wall_s);
                }
                if (!to_a && !to_b) {
                    // A logo with alpha composites by its alpha; an opaque
                    // texture keeps the screen blend the demo chose. Decided
                    // here, once, where the clip arrives.
                    engine.overlay_layer().blend =
                        header.has_alpha() ? BlendMode::Alpha : BlendMode::Screen;
                    engine.overlay_layer().enabled = true;
                    view.overlay_live = false;
                }
                rebuild_passes();
                view.last_error.clear();
            } else {
                view.last_error = entry.name + " : " + error;
            }
        }
        view.load_clip = kNoClip;
        view.load_keep_position = false;

        // Persist what changed. The demo's fabricated library is never written.
        if (view.library_dirty && library_is_real) {
            std::string error;
            if (!library_save(library_snapshot(engine.library(), engine.matrix()), kLibraryFile,
                              error)) {
                view.last_error = error;
            }
            view.library_dirty = false;
        }
        view.library_dirty = false;
        if (view.settings_dirty) {
            std::string error;
            if (!settings_save(desk, "settings.json", error)) view.last_error = error;
            // What the settings drive live: the window budget, and the wire.
            engine.set_window_budget(static_cast<std::uint64_t>(desk.vram_budget_mb) << 20);
            const std::string endpoint = desk.control_host + ":" + std::to_string(desk.control_port);
            if (endpoint != control_endpoint) {
                control.close();
                control.open(desk.control_host, static_cast<std::uint16_t>(desk.control_port));
                control_endpoint = endpoint;
            }
            view.settings_dirty = false;
        }
        view.analysis_pending = analysis.pending();
        view.analysis_busy = analysis.busy();
        view.share_open = share.active();

        const auto now_us = static_cast<std::uint64_t>(wall_s * 1e6);

        // The sound the script is notionally playing, into the reactive bands.
        // Synthetic until an audio device exists -- but the analyser, the
        // mapping and everything downstream are the real ones, so what is on
        // screen is the actual behaviour rather than a mock of it. Skipped on
        // the frame the script wraps, where the span would run backwards.
        if (view.script_running && t > span_from) {
            static std::vector<float> audio(4096);
            const std::size_t written =
                simulation.audio(span_from, t, kBpm, 48000.0, audio.data(), audio.size());
            engine.analyse_audio(audio.data(), written);
        }

        // The script writes the surface and moves the platters. Frozen, it does
        // neither -- so what is on screen is what the hand did, and nothing else.
        if (view.script_running) simulation.step(t, engine.surface(), now_us);
        for (const auto& owned : hand.owned) {
            engine.surface().set(owned.first, owned.second, now_us);
        }

        // --- the mixer -------------------------------------------------------
        // After the script and the mouse, so a real control always wins: that is
        // the same rule the hand already follows, and what makes plugging the
        // mixer in mid-demo feel like taking over rather than fighting.
        if (view.rig_reconfigure_request) {
            // The devices changed in the settings: rebuild the rig, declare
            // what the new profiles have, and re-resolve the mappings.
            declare_rig();
            rig.configure(desk.devices, loaded_profiles);
            engine.bind();
            view.rig_reconfigure_request = false;
        }
        if (view.learn_start && !learn.active()) {
            const std::size_t which = static_cast<std::size_t>(std::max(0, view.learn_device));
            if (which < desk.devices.size()) {
                const DeviceProfile* p = find_profile(desk.devices[which].profile, loaded_profiles);
                if (p != nullptr) {
                    const DeviceProfile profile =
                        p->name == "rp8000" ? rp8000_profile(desk.devices[which].deck) : *p;
                    learn.begin(profile_targets(profile));
                    learn_device = static_cast<int>(which);
                }
            }
            view.learn_start = false;
        }
        if (view.learn_cancel) {
            // Keep what was learned; a run abandoned half way is still worth its
            // first half.
            while (learn.active()) learn.skip();
            view.learn_cancel = false;
        }
        if (view.learn_skip && learn.active()) {
            learn.skip();
            view.learn_skip = false;
        }

        // Retried while absent, because a DJ does not power the rig in the order
        // an application would prefer: the mixer switched on after the window
        // opened would otherwise never be seen. Once a second, so a missing port
        // costs nothing. A device that comes back has its knobs forgotten:
        // the values on screen were the script's, and the knobs are wherever
        // they physically are. Nothing is known until each one moves.
        if (!rig.poll(wall_s).empty()) engine.surface().forget_positions();

        rig.drain(midi_events);
        for (const MidiEvent& midi_event : midi_events) {
            if (learn.active()) {
                // While learning, events bind rather than move: a sweep that
                // also drove the control it is teaching would be confusing to
                // watch and would fight the script. And only the device being
                // learned binds: a pad pressed on a turntable must not become
                // the mixer's next knob.
                if (static_cast<int>(midi_event.device) == learn_device) learn.observe(midi_event);
            } else {
                engine.surface().apply(midi_event, now_us);
            }
        }

        const bool was_learning = view.learning;
        view.learning = learn.active();
        view.learn_device = learn_device;
        if (was_learning && !view.learning) {
            // The run finished or was abandoned. Apply and persist in one place,
            // so a learned surface survives the application closing -- which is
            // the whole point of learning it. The other devices' bindings are
            // kept: this run was one device's.
            learn.apply_to(engine.surface());
            std::vector<LearnResult> kept;
            for (const LearnResult& b : surface_config.bindings) {
                if (static_cast<int>(b.address.device) != learn_device) kept.push_back(b);
            }
            for (const LearnResult& b : learn.results()) kept.push_back(b);
            surface_config.bindings = std::move(kept);
            surface_config.mappings.clear();
            for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
                surface_config.mappings.push_back(engine.mapping().at(i));
            }
            std::string error;
            if (!config_save(surface_config, "mapping.json", error)) view.last_error = error;
            engine.bind();
            learn_device = -1;
        }
        if (view.learning) {
            view.learn_prompt = learn.current().id;
            view.learn_remaining = learn.remaining();
        }
        view.midi_connected = rig.connected_count() > 0;
        view.midi_port = rig.connected_names();
        view.midi_messages = rig.message_count();
        view.midi_total = engine.surface().size();
        view.rig.clear();
        for (std::size_t i = 0; i < rig.size(); ++i) {
            svj::ui::Frame::RigDeviceView d;
            d.profile = rig.at(i).profile;
            d.profile_name = rig.at(i).setting.profile;
            d.port = rig.connected(i) ? rig.at(i).input->port_name() : rig.at(i).setting.port;
            d.connected = rig.connected(i);
            d.deck = rig.at(i).setting.deck;
            view.rig.push_back(std::move(d));
        }
        // Once a second is plenty for the settings' list of ports.
        static double last_ports_s = -10.0;
        if (wall_s - last_ports_s >= 1.0) {
            last_ports_s = wall_s;
            view.midi_ports = svj::ui::MidiInput::ports();
        }
        // The mixer's switches, wherever they were set (the surface's chips or
        // a mapped button), are the desk's settings.
        if (engine.mix_settings().xfader != desk.mix.xfader ||
            engine.mix_settings().channel != desk.mix.channel ||
            engine.mix_settings().xfader_reverse != desk.mix.xfader_reverse ||
            engine.mix_settings().channel_reverse != desk.mix.channel_reverse) {
            desk.mix = engine.mix_settings();
            view.settings_dirty = true;
        }
        // BOUND, not "known". A control the demo script moves is known and still
        // has no MIDI address, so counting known ones would claim a mixer had
        // been learned when nothing had been.
        view.midi_bound = surface_config.bindings.size();

        // The platter's source, served at the frame boundary. Opening an audio
        // device is I/O, so the panel only asks and this is where it happens.
        if (view.deck_a_live && !platter_in.ready()) {
            if (platter_in.open(desk.platter_endpoint, desk.platter_first_channel)) {
                QuadratureConfig config;
                config.carrier_hz = desk.carrier_hz;
                config.sample_rate = platter_in.sample_rate();
                if (!platter.configure(config)) {
                    std::fprintf(stderr, "plateau live: configuration refusee\n");
                    platter_in.close();
                    view.deck_a_live = false;
                } else {
                    // Tell the deck what this source can promise: no absolute
                    // position (a re-lock is a jump, so the anchor ages), and a
                    // speed limit that is the tracker's own Nyquist figure --
                    // 22.05x on this 44.1 kHz interface, not the 24x the deck
                    // assumed for a control record at 48 kHz.
                    engine.deck_a().timecode.set_source(false, platter.max_speed_ratio());
                }
            } else {
                std::fprintf(stderr, "plateau live: entree \"%s\" introuvable\n",
                             desk.platter_endpoint.c_str());
                view.deck_a_live = false;
            }
        } else if (!view.deck_a_live && platter_in.ready()) {
            platter_in.close();
            // Back to the script, which reads an absolute position off its
            // virtual record.
            engine.deck_a().timecode.set_source(true, TimecodeConfig{}.max_speed_ratio);
        }

        EngineFrame frame;
        frame.time_s = wall_s;
        frame.dt_s = dt;
        frame.now_us = now_us;
        frame.deck_a = simulation.deck_a();
        frame.deck_b = simulation.deck_b();

        if (!view.script_running) {
            // A platter standing still, NOT a lost link. Reporting a dropout
            // would put the decks in Degraded and coast the picture on the last
            // pitch; on real hardware a stopped record still carries its
            // carrier. Hold each deck exactly where it is.
            const auto hold = [wall_s](const Deck& deck) {
                DecoderSample sample;
                sample.time_s = wall_s;
                sample.position_s = deck.timecode.state().position_s;
                sample.pitch = 0.0f;
                sample.signal_level = 1.0f;
                sample.locked = true;
                return sample;
            };
            frame.deck_a = hold(engine.deck_a());
            frame.deck_b = hold(engine.deck_b());
        }

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
        // Loop, cue and slip edges: the script's while it runs, and whatever
        // the pads and the loop row asked last frame. Both are buttons.
        if (view.script_running) frame.commands_a = to_commands(simulation.events());
        const auto merge = [](DeckCommands& into, const DeckCommands& from) {
            into.loop_in = into.loop_in || from.loop_in;
            into.loop_out = into.loop_out || from.loop_out;
            into.loop_exit = into.loop_exit || from.loop_exit;
            into.slip_on = into.slip_on || from.slip_on;
            into.slip_off = into.slip_off || from.slip_off;
            if (from.cue_jump || from.cue_set || from.cue_clear) {
                into.cue_jump = from.cue_jump;
                into.cue_set = from.cue_set;
                into.cue_clear = from.cue_clear;
                into.cue_index = from.cue_index;
            }
            if (from.auto_loop_beats > 0.0) into.auto_loop_beats = from.auto_loop_beats;
            if (from.beat_jump_beats != 0.0) into.beat_jump_beats = from.beat_jump_beats;
        };
        // The take drives the surface and the platters in place of the
        // desk: every record up to this moment is applied, the last one wins.
        if (view.take_replaying) {
            const std::uint64_t elapsed_us =
                static_cast<std::uint64_t>((wall_s - replay_started_s) * 1e6);
            bool ended = false;
            for (;;) {
                if (!replay_state_pending) {
                    svj::PacketKind kind{};
                    svj::SchemaPacket schema;
                    std::string error;
                    if (!replay.next(kind, replay_state, schema, error)) {
                        ended = true;
                        if (!error.empty()) view.last_error = "prise : " + error;
                        break;
                    }
                    if (kind != svj::PacketKind::State) continue;
                    if (!replay_have_first) {
                        replay_first_us = replay_state.t_us;
                        replay_have_first = true;
                    }
                    replay_state_pending = true;
                    ++replay_read;
                }
                if (replay_state.t_us - replay_first_us > elapsed_us) break;  // not yet
                for (std::size_t i = 0; i < replay_controls.size() && i < replay_state.values.size(); ++i) {
                    if (replay_controls[i] == kNoControl || !replay_state.known[i]) continue;
                    engine.surface().set(replay_controls[i], replay_state.values[i], now_us);
                }
                const auto sample = [&](const svj::DeckWire& wire) {
                    DecoderSample s;
                    s.time_s = wall_s;
                    s.position_s = static_cast<double>(wire.pos_s);
                    s.pitch = wire.velocity;
                    s.signal_level = 1.0f;
                    s.locked = true;
                    return s;
                };
                frame.deck_a = sample(replay_state.deck_a);
                frame.deck_b = sample(replay_state.deck_b);
                replay_state_pending = false;
            }
            view.take_replay_progress = replay.record_count() > 1
                                            ? static_cast<float>(replay_read) /
                                                  static_cast<float>(replay.record_count() - 1)
                                            : 1.0f;
            if (ended) {
                replay.close();
                view.take_replaying = false;
                for (Deck* deck : {&engine.deck_a(), &engine.deck_b()}) deck->play(wall_s);
                view.notice = "relecture termin\xC3\xA9""e";
                notice_until_s = wall_s + 4.0;
            }
        }
        merge(frame.commands_a, view.commands_a);
        merge(frame.commands_b, view.commands_b);
        view.commands_a = DeckCommands{};
        view.commands_b = DeckCommands{};
        engine.step(frame);

        // What a mapped pad asked of the front end this step: served NEXT
        // frame boundary through the same requests a click writes, so a pad
        // and a click cannot disagree about what "next" means.
        {
            const EngineRequests asked = engine.take_requests();
            const auto queue_onto = [&](DeckTarget target) {
                const QueueItem next = engine.queue().take_next();
                if (next.clip == kNoClip || !engine.library().at(next.clip).playable()) return;
                view.load_clip = next.clip;
                view.load_target = target;
            };
            if (asked.load_next_a) queue_onto(DeckTarget::A);
            if (asked.load_next_b) queue_onto(DeckTarget::B);
            if (asked.load_next_overlay) queue_onto(DeckTarget::Overlay);
            if (asked.library_step != 0 && engine.library().size() > 0) {
                const int count = static_cast<int>(engine.library().size());
                int cursor = view.library_cursor < 0 ? 0 : view.library_cursor + asked.library_step;
                cursor = std::clamp(cursor, 0, count - 1);
                view.library_cursor = cursor;
            }
            const auto load_cursor = [&](DeckTarget target) {
                if (view.library_cursor < 0 ||
                    static_cast<std::size_t>(view.library_cursor) >= engine.library().size()) {
                    return;
                }
                if (!engine.library().at(view.library_cursor).playable()) return;
                view.load_clip = view.library_cursor;
                view.load_target = target;
            };
            if (asked.library_load_a) load_cursor(DeckTarget::A);
            if (asked.library_load_b) load_cursor(DeckTarget::B);
            if (asked.library_load_overlay) load_cursor(DeckTarget::Overlay);
        }
        // The browse encoder walks the library by its detents, whoever it is
        // on. The profile says which control that is; the id does.
        if (const ControlIndex browse = engine.surface().find("browse.encoder"); browse != kNoControl) {
            const int ticks = engine.surface().take_ticks(browse);
            if (ticks != 0 && engine.library().size() > 0) {
                const int count = static_cast<int>(engine.library().size());
                const int cursor = view.library_cursor < 0 ? 0 : view.library_cursor + ticks;
                view.library_cursor = std::clamp(cursor, 0, count - 1);
            }
        }

        // --- the take ------------------------------------------------------------
        if (view.take_toggle_request) {
            view.take_toggle_request = false;
            std::string error;
            if (view.take_recording) {
                if (!take.close(error)) view.last_error = error;
                view.take_recording = false;
                view.notice = "prise enregistr\xC3\xA9""e : " + view.take_name;
                notice_until_s = wall_s + 6.0;
            } else {
                std::error_code ec;
                std::filesystem::create_directories("takes", ec);
                char stamp[32];
                const std::time_t now = std::time(nullptr);
                std::tm local{};
#if defined(_WIN32)
                localtime_s(&local, &now);
#else
                localtime_r(&now, &local);
#endif
                std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &local);
                view.take_name = std::string("takes/") + stamp + ".svtake";
                if (take.open(view.take_name, error) && take.write_schema(wire_schema, error)) {
                    view.take_recording = true;
                } else {
                    view.last_error = "prise : " + error;
                }
            }
        }
        if (view.take_recording) {
            std::string error;
            if (!take.write_state(engine.packet(now_us, wire_schema.schema_hash), error)) {
                view.last_error = "prise : " + error;
                take.close(error);
                view.take_recording = false;
            }
            view.take_records = take.records_written();
        }

        // --- replaying a take ------------------------------------------------------
        if (view.take_list_request) {
            view.take_list_request = false;
            view.takes.clear();
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator("takes", ec)) {
                if (entry.path().extension() == ".svtake") view.takes.push_back(entry.path().string());
            }
            std::sort(view.takes.rbegin(), view.takes.rend());
        }
        if (!view.take_replay_request.empty()) {
            std::string error;
            replay.close();
            if (replay.open(view.take_replay_request, error)) {
                // The schema comes first; every later record is a state.
                svj::PacketKind kind{};
                svj::SchemaPacket schema;
                svj::StatePacket state;
                replay_controls.clear();
                if (replay.next(kind, state, schema, error) && kind == svj::PacketKind::Schema) {
                    for (const svj::SchemaEntry& entry : schema.entries) {
                        replay_controls.push_back(engine.surface().find(entry.id));
                    }
                }
                view.take_replaying = true;
                view.take_replay_name = view.take_replay_request;
                replay_started_s = wall_s;
                replay_have_first = false;
                replay_read = 0;
                replay_state_pending = false;
                // The decks follow the take's platters, as they followed the
                // real ones when it was recorded.
                view.script_running = false;
                for (Deck* deck : {&engine.deck_a(), &engine.deck_b()}) {
                    deck->clock.set_source(DeckSource::Timecode, wall_s);
                }
                view.notice = "relecture : " + view.take_replay_name;
                notice_until_s = wall_s + 4.0;
            } else {
                view.last_error = "prise : " + error;
            }
            view.take_replay_request.clear();
        }
        if (view.take_replaying && view.take_replay_stop) {
            replay.close();
            view.take_replaying = false;
            for (Deck* deck : {&engine.deck_a(), &engine.deck_b()}) deck->play(wall_s);
        }
        view.take_replay_stop = false;

        // --- the anchor, and the mapping list ------------------------------------
        if (view.anchor_request) {
            view.anchor_request = false;
            engine.anchor_now(wall_s);
        }
        if (view.mapping_listen) {
            // The next control that moves names the selected row's source.
            const ControlIndex touched = engine.surface().last_touched();
            if (touched != kNoControl && view.mapping_selected >= 0 &&
                static_cast<std::size_t>(view.mapping_selected) < engine.mapping().size()) {
                Mapping& row = engine.mapping().mutable_at(static_cast<std::size_t>(view.mapping_selected));
                const std::string& id = engine.surface().at(touched).id;
                if (row.source.kind != SourceKind::Control || row.source.control_id != id) {
                    row.source.kind = SourceKind::Control;
                    row.source.control_id = id;
                    view.mappings_dirty = true;
                    view.mapping_listen = false;
                }
            }
        }
        if (view.mappings_dirty) {
            view.mappings_dirty = false;
            for (const std::string& id : engine.bind()) view.last_error = "liaison inconnue : " + id;
            surface_config.mappings.clear();
            for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
                surface_config.mappings.push_back(engine.mapping().at(i));
            }
            std::string error;
            if (!config_save(surface_config, "mapping.json", error)) view.last_error = error;
        }

        // The control stream: state every frame, schema once a second so a
        // client attaching mid-set still learns what the floats mean.
        control.send_state(engine.packet(now_us, wire_schema.schema_hash));
        if (wall_s - last_schema_sent_s >= 1.0) {
            control.send_schema(wire_schema);
            last_schema_sent_s = wall_s;
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        view.elapsed_s = wall_s;
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
            // The room's screen gets the SAME texture the preview shows and
            // the readback publishes, so the three cannot drift apart.
            // The room's screen gets the SAME texture the preview shows and
            // the readback publishes -- through the geometry SORTIE sets.
            // Spout gets it before the geometry: a receiver does its own.
            svj::ui::OutputGeometry geometry;
            geometry.pin = &engine.pin();
            geometry.mesh = &engine.mesh();
            geometry.mesh_enabled = engine.mesh_enabled();
            geometry.mask = &engine.mask();
            output.present(shown, gpu.width(), gpu.height(), geometry);
            view.program_width = gpu.width();
            view.program_height = gpu.height();
        }
        view.scrubbing = nullptr;
        thumbnails.sync(engine.library());
        view.thumbnails = thumbnails.textures();
        svj::ui::draw(engine, view);

        // Whatever no widget claimed this frame is not being held. Hand it back
        // to its platter through the Grab takeover, so the picture stays put.
        // Released through the DECK, which remembers where the position came
        // from; and at wall time, which is the clock the engine runs on -- the
        // script's wrapped `t` is not, and a seek at the wrong time moved the
        // picture by the difference.
        for (Deck* deck : {&engine.deck_a(), &engine.deck_b()}) {
            if (deck->clock.source() == DeckSource::Hand && view.scrubbing != deck) {
                deck->release(wall_s);
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

    if (view.take_recording) {
        std::string error;
        take.close(error);
    }
    replay.close();
    output.close();
    thumbnails.clear();
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
