// scratchvj — holds the multi-tap shader to the plan core/videotaps produces.
//
// Eight layers, eight known colours, and the two things the shader claims:
// trails are the weighted sum the plan carries, and a slit scan shows band k
// from layer k. Both are checked against arithmetic done here, so a shader that
// samples the wrong layer -- the fault that looks like "roughly right" on a
// moving picture and is impossible to see by eye -- fails loudly instead.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/videotaps.h"
#include "gpu_taps.h"

using namespace svj;

namespace {

constexpr std::uint32_t kW = 64;
constexpr std::uint32_t kH = 32;

// Layer k is a flat grey of value 20 + 30k, so which layers a pixel came from
// is readable straight off the number.
std::uint8_t layer_value(int k) { return static_cast<std::uint8_t>(20 + 30 * k); }

std::uint16_t build_layers() {
    const bgfx::TextureHandle handle = bgfx::createTexture2D(
        static_cast<std::uint16_t>(kW), static_cast<std::uint16_t>(kH), false,
        static_cast<std::uint16_t>(kTapCount), bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT);
    if (!bgfx::isValid(handle)) return 0xFFFF;

    std::vector<std::uint8_t> layer(static_cast<std::size_t>(kW) * kH * 4, 255);
    for (int k = 0; k < kTapCount; ++k) {
        for (std::size_t i = 0; i < layer.size(); i += 4) {
            layer[i] = layer[i + 1] = layer[i + 2] = layer_value(k);
        }
        bgfx::updateTexture2D(handle, static_cast<std::uint16_t>(k), 0, 0, 0,
                              static_cast<std::uint16_t>(kW),
                              static_cast<std::uint16_t>(kH),
                              bgfx::copy(layer.data(),
                                         static_cast<std::uint32_t>(layer.size())));
    }
    return handle.idx;
}

EffectUnit delay_unit() {
    EffectUnit unit;
    unit.type = EffectType::Delay;
    unit.enabled = true;
    unit.shared.mix = 1.0f;
    unit.shared.feedback = 0.7f;
    unit.sync.tempo = true;
    unit.sync.beats = 0.5;
    return unit;
}

}  // namespace

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("taps_check", 128, 128, SDL_WINDOW_HIDDEN);
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

    const std::uint16_t layers = build_layers();
    if (layers == 0xFFFF) {
        std::fprintf(stderr, "tableau de textures indisponible\n");
        return 1;
    }

    svj::ui::TapsGpu pass;
    if (!pass.init(kW, kH, 0)) {
        std::fprintf(stderr, "passe taps: init a echoue\n");
        return 1;
    }

    bgfx::TextureHandle readback = bgfx::createTexture2D(
        static_cast<std::uint16_t>(kW), static_cast<std::uint16_t>(kH), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kW) * kH * 4);

    const auto run = [&](const EffectUnit& unit, const TapPlan& plan) {
        const std::uint16_t result = pass.render(layers, 0xFFFF, unit, plan);
        bgfx::TextureRegion destination;
        destination.init(readback, 0, 0);
        bgfx::TextureHandle from;
        from.idx = result;
        bgfx::TextureRegion origin;
        origin.init(from, 0, 0);
        bgfx::blit(1, destination, origin);
        bgfx::TextureRegion whole;
        whole.init(readback, 0, 0);
        const std::uint32_t ready = bgfx::read(whole, pixels.data());
        while (bgfx::frame() < ready) {
        }
        return result;
    };

    int failures = 0;

    // --- trails: the weighted sum the plan carries ---------------------------
    {
        const EffectUnit unit = delay_unit();
        const TapPlan plan =
            plan_taps(unit, 100.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
        run(unit, plan);

        double expected = 0.0;
        for (int k = 0; k < plan.count; ++k) expected += plan.weight[k] * layer_value(k);

        const int got = pixels[(static_cast<std::size_t>(kH / 2) * kW + kW / 2) * 4];
        const double error = std::abs(got - expected);
        std::printf("%-16s : attendu %.1f, obtenu %d\n", "trainees", expected, got);
        if (error > 2.0) ++failures;
    }

    // --- slit scan: band k must show layer k, not a blend --------------------
    {
        EffectUnit unit;
        unit.type = EffectType::SlitScan;
        unit.enabled = true;
        unit.shared.mix = 1.0f;
        unit.shared.time = 0.5f;
        const TapPlan plan =
            plan_taps(unit, 100.0, 1.0, 0.5, 300.0, ClipPlayMode::Loop);
        run(unit, plan);

        int wrong = 0;
        for (int band = 0; band < kTapCount; ++band) {
            // The middle of band `band`, so a half-texel of rounding cannot
            // move the sample into its neighbour.
            const std::uint32_t x = static_cast<std::uint32_t>(
                (band + 0.5) * kW / kTapCount);
            const int got = pixels[(static_cast<std::size_t>(kH / 2) * kW + x) * 4];
            if (std::abs(got - layer_value(band)) > 2) ++wrong;
        }
        std::printf("%-16s : %d bandes sur %d hors couche\n", "slit scan", wrong,
                    kTapCount);
        if (wrong > 0) ++failures;
    }

    // --- a stopped record draws nothing, and says so -------------------------
    {
        const EffectUnit unit = delay_unit();
        const TapPlan plan = plan_taps(unit, 100.0, 0.0, 0.5, 300.0, ClipPlayMode::Loop);
        const std::uint16_t result = pass.render(layers, 0x1234, unit, plan);
        if (!plan.collapsed || result != 0x1234) {
            std::fprintf(stderr, "un disque arrete a quand meme dessine une trainee\n");
            ++failures;
        } else {
            std::printf("%-16s : aucune passe\n", "disque arrete");
        }
    }

    pass.destroy();
    bgfx::destroy(readback);
    bgfx::destroy(bgfx::TextureHandle{layers});
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d cas hors tolerance : la passe multi-taps a devie\n",
                     failures);
        return 2;
    }
    std::printf("passe multi-taps conforme a core/videotaps\n");
    return 0;
}
