// scratchvj — holds the video effect shader to core/videofx.
//
// Same pictures through both paths, every single-frame effect, at several knob
// positions, compared channel for channel. The rack is what the instrument's
// whole thesis rests on -- one knob moving the sound and the picture by the
// same equation -- so an effect drifting from its reference is not a cosmetic
// fault, it is the claim quietly becoming untrue.
//
// Tolerance is 2/255: both paths round to eight bits, and the GPU's exp() need
// not agree with the CPU's in the last place. Anything above rounding is a real
// divergence and fails.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/effect.h"
#include "core/videofx.h"
#include "gpu_effects.h"

using namespace svj;

namespace {

constexpr std::uint32_t kW = 64;
constexpr std::uint32_t kH = 64;

// Asymmetric on both axes and full of detail at several scales, so a flip, a
// channel swap or a wrong blur radius all show up.
std::vector<std::uint8_t> source_image() {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(kW) * kH * 4);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            std::uint8_t* p = image.data() + (static_cast<std::size_t>(y) * kW + x) * 4;
            const bool fine = ((x / 2) + (y / 3)) % 2 == 0;
            p[0] = static_cast<std::uint8_t>(fine ? 40 + x * 3 : 200 - y * 2);
            p[1] = static_cast<std::uint8_t>((x * 4 + y) & 0xFF);
            p[2] = static_cast<std::uint8_t>(fine ? 230 - x : 30 + y * 3);
            p[3] = 255;
        }
    }
    return image;
}

int worst_difference(const std::uint8_t* gpu, const std::vector<std::uint8_t>& cpu) {
    int worst = 0;
    for (std::size_t i = 0; i < cpu.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            const int d = std::abs(static_cast<int>(gpu[i + c]) -
                                   static_cast<int>(cpu[i + c]));
            if (d > worst) worst = d;
        }
    }
    return worst;
}

const char* name_of(EffectType type) {
    const EffectDescriptor* d = describe(type);
    return d != nullptr ? d->id : "?";
}

}  // namespace

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("fx_check", 128, 128, SDL_WINDOW_HIDDEN);
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

    const std::vector<std::uint8_t> source = source_image();
    const std::uint16_t source_tex =
        bgfx::createTexture2D(static_cast<std::uint16_t>(kW), static_cast<std::uint16_t>(kH),
                              false, 1, bgfx::TextureFormat::RGBA8,
                              BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT,
                              bgfx::copy(source.data(),
                                         static_cast<std::uint32_t>(source.size())))
            .idx;

    svj::ui::EffectsGpu effects;
    if (!effects.init(kW, kH, 0)) {
        std::fprintf(stderr, "effets GPU: init a echoue\n");
        return 1;
    }

    bgfx::TextureHandle readback = bgfx::createTexture2D(
        static_cast<std::uint16_t>(kW), static_cast<std::uint16_t>(kH), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kW) * kH * 4);

    const EffectType kinds[] = {EffectType::LowPass,   EffectType::HighPass,
                                EffectType::Bitcrusher, EffectType::Invert,
                                EffectType::Mirror,     EffectType::Kaleidoscope};
    const float amounts[] = {0.15f, 0.6f, 1.0f};

    int failures = 0;
    for (const EffectType kind : kinds) {
        int worst_for_kind = 0;
        for (const float amount : amounts) {
            const VideoFx fx{kind, 1.0f, amount};

            std::vector<std::uint8_t> expected;
            apply_video_fx(source.data(), kW, kH, fx, expected);

            // Through the rack's own path, so what is validated is the code the
            // application runs rather than a convenient shortcut past it.
            EffectRack rack(1);
            rack.load(0, kind);
            rack.at(0).shared.mix = 1.0f;
            rack.at(0).shared.amount = amount;
            const std::uint16_t result = effects.render(source_tex, rack);

            bgfx::TextureRegion destination;
            destination.init(readback, 0, 0);
            bgfx::TextureHandle from;
            from.idx = result;
            bgfx::TextureRegion origin;
            origin.init(from, 0, 0);
            bgfx::blit(static_cast<std::uint16_t>(svj::ui::kMaxEffectPasses), destination,
                       origin);
            bgfx::TextureRegion whole;
            whole.init(readback, 0, 0);
            const std::uint32_t ready = bgfx::read(whole, pixels.data());
            while (bgfx::frame() < ready) {
            }

            const int worst = worst_difference(pixels.data(), expected);
            if (worst > worst_for_kind) worst_for_kind = worst;
            if (worst > 2) ++failures;
        }
        std::printf("%-14s : ecart max %d\n", name_of(kind), worst_for_kind);
    }

    // And the property the chain itself must have: an idle rack changes nothing.
    {
        EffectRack rack(3);
        const std::uint16_t result = effects.render(source_tex, rack);
        if (result != source_tex || effects.passes() != 0) {
            std::fprintf(stderr, "un rack au repos a quand meme dessine\n");
            ++failures;
        } else {
            std::printf("%-14s : aucune passe\n", "rack au repos");
        }
    }

    effects.destroy();
    bgfx::destroy(readback);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d cas hors tolerance : un effet a quitte sa reference\n",
                     failures);
        return 2;
    }
    std::printf("effets video GPU conformes a core/videofx\n");
    return 0;
}
