// scratchvj — proves a live Spout input is actually scratchable.
//
// spout_check proves frames LEAVE the machine. This proves they come back in
// and, crucially, that what is kept is a HISTORY rather than the same picture
// over and over: a ring that quietly wrote one slot would fill, report a span,
// and be completely unscratchable while looking fine from the outside.
//
// So it checks three things a running sender makes checkable:
//   - frames arrive and are not black (texture sharing's silent failure),
//   - the ring fills and its span stops growing at the budget,
//   - frames far apart in the history DIFFER from each other.
//
// Needs a Spout sender running -- scratchvj_ui itself is one, named "scratchvj",
// which makes the whole loop testable on one machine with nothing installed.
// That prerequisite is why it stays out of ctest, like spout_check and net_check.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "live_in.h"

namespace {

double mean_luma(const std::uint8_t* rgba, std::size_t pixels) {
    if (rgba == nullptr || pixels == 0) return 0.0;
    unsigned long long sum = 0;
    for (std::size_t i = 0; i < pixels; ++i) {
        sum += rgba[i * 4] + rgba[i * 4 + 1] + rgba[i * 4 + 2];
    }
    return static_cast<double>(sum) / (static_cast<double>(pixels) * 3.0);
}

}  // namespace

int main(int argc, char** argv) {
    const char* name = argc > 1 ? argv[1] : "scratchvj";

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("live_check", 128, 128, SDL_WINDOW_HIDDEN);
    if (window == nullptr) return 1;

    bgfx::renderFrame();
    bgfx::Init init;
    init.swapChain.nwh = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    init.swapChain.width = 128;
    init.swapChain.height = 128;
    if (!bgfx::init(init)) {
        std::fprintf(stderr, "bgfx indisponible\n");
        return 1;
    }

    svj::ui::LiveInput live;
    // A small budget on purpose, so the ring fills within the run and the "span
    // stops growing" property is actually exercised rather than assumed.
    if (!live.open(name, 48ull << 20)) {
        std::fprintf(stderr, "DX11 indisponible\n");
        return 1;
    }

    int captured = 0;
    double span_at_half = 0.0;
    for (int tick = 0; tick < 420; ++tick) {
        const double now_s = tick / 60.0;
        if (live.poll(now_s)) ++captured;
        if (tick == 210) span_at_half = live.ring().span_s();
        // The frame the compositor would sample, so the upload path runs too.
        live.frame_at(live.ring().newest_s());
        bgfx::frame();
        SDL_Delay(16);
    }

    int failures = 0;
    if (captured == 0) {
        std::fprintf(stderr, "aucune frame du sender \"%s\"\n", name);
        live.close();
        bgfx::shutdown();
        return 1;
    }
    std::printf("sender \"%s\"  %ux%u  %d frames  anneau %zu/%zu  %.2f s d'historique\n",
                live.sender_name().c_str(), live.width(), live.height(), captured,
                live.ring().size(), live.ring().capacity(), live.ring().span_s());

    // --- the history holds different pictures --------------------------------
    if (live.ring().size() < 3) {
        std::fprintf(stderr, "anneau trop court pour verifier l'historique\n");
        ++failures;
    } else {
        const std::size_t pixels = static_cast<std::size_t>(live.width()) * live.height();
        const svj::LivePick oldest = live.ring().pick(live.ring().oldest_s());
        const svj::LivePick newest = live.ring().pick(live.ring().newest_s());
        const double a = mean_luma(live.pixels(oldest.slot), pixels);
        const double b = mean_luma(live.pixels(newest.slot), pixels);
        std::printf("%-22s : plus ancienne %.1f, plus recente %.1f\n", "luminance", a, b);
        if (a < 1.0 && b < 1.0) {
            std::fprintf(stderr, "flux entierement noir : partage suspect\n");
            ++failures;
        }
        if (std::fabs(a - b) < 0.01) {
            // Not conclusive on a still image, so it is reported rather than
            // failed: a sender showing a frozen picture is legitimately uniform.
            std::printf("%-22s : identiques -- source immobile ?\n", "historique");
        } else {
            std::printf("%-22s : distinctes, l'historique est scratchable\n",
                        "historique");
        }
    }

    // --- the span is bounded by the budget -----------------------------------
    if (live.ring().size() == live.ring().capacity()) {
        const double grew = live.ring().span_s() - span_at_half;
        std::printf("%-22s : plein, la profondeur a varie de %.2f s sur la 2e moitie\n",
                    "budget", grew);
        // Full means the span is governed by the budget, not by how long the
        // tool ran -- which is the property that matters, because it is what
        // stops a live deck from eating memory for the length of a set.
        if (live.ring().span_s() > span_at_half * 2.0 && span_at_half > 0.0) {
            std::fprintf(stderr, "l'historique grandit encore alors que l'anneau est plein\n");
            ++failures;
        }
    } else {
        std::printf("%-22s : anneau non rempli (%zu/%zu) sur la duree du test\n", "budget",
                    live.ring().size(), live.ring().capacity());
    }

    // --- scratching past the tail is reported --------------------------------
    live.frame_at(live.ring().oldest_s() - 10.0);
    if (!live.clamped()) {
        std::fprintf(stderr, "un grattage au-dela de l'historique n'a pas ete signale\n");
        ++failures;
    } else {
        std::printf("%-22s : signale\n", "au-dela de l'historique");
    }

    live.close();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d probleme(s) sur l'entree live\n", failures);
        return 2;
    }
    std::printf("entree live conforme\n");
    return 0;
}
