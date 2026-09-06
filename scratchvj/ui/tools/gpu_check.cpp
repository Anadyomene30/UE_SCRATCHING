// scratchvj — holds the GPU compositor to its CPU reference.
//
// core/compose is tested arithmetic; fs_program.sc claims to be the same
// arithmetic. This tool makes the claim falsifiable: same three synthetic
// layers through both paths, every blend mode, and the outputs must agree
// channel for channel within rounding. A flipped image, a swapped channel, a
// blend equation drifting from its reference -- each fails loudly here, on a
// hidden window, with no eyeball in the loop.
//
// Tolerance is 2/255 per channel: the CPU path rounds once at the end, the GPU
// rounds through an 8-bit render target, and the two may land a hair apart.
// Anything larger than rounding is a real divergence and must fail.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/compose.h"
#include "gpu_compose.h"

using namespace svj;

namespace {

constexpr std::uint32_t kSize = 64;

// Asymmetric on both axes, so a vertical or horizontal flip cannot pass.
std::vector<std::uint8_t> pattern(int seed) {
    std::vector<std::uint8_t> image(kSize * kSize * 4);
    for (std::uint32_t y = 0; y < kSize; ++y) {
        for (std::uint32_t x = 0; x < kSize; ++x) {
            std::uint8_t* p = image.data() + (y * kSize + x) * 4;
            p[0] = static_cast<std::uint8_t>((x * 4 + seed * 37) & 0xFF);
            p[1] = static_cast<std::uint8_t>((y * 4 + seed * 91) & 0xFF);
            p[2] = static_cast<std::uint8_t>((x * 2 + y * 3 + seed * 53) & 0xFF);
            p[3] = static_cast<std::uint8_t>((x * 3 + y + 128) & 0xFF);
        }
    }
    return image;
}

std::uint16_t upload(const std::vector<std::uint8_t>& image) {
    return bgfx::createTexture2D(
               kSize, kSize, false, 1, bgfx::TextureFormat::RGBA8,
               BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT,
               bgfx::copy(image.data(), static_cast<std::uint32_t>(image.size())))
        .idx;
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

}  // namespace

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window =
        SDL_CreateWindow("gpu_check", 128, 128, SDL_WINDOW_HIDDEN);
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

    const auto layer_a = pattern(1);
    const auto layer_b = pattern(2);
    const auto layer_o = pattern(3);
    const std::uint16_t tex_a = upload(layer_a);
    const std::uint16_t tex_b = upload(layer_b);
    const std::uint16_t tex_o = upload(layer_o);

    svj::ui::ProgramGpu gpu;
    if (!gpu.init(kSize, kSize, 0, 1)) {
        std::fprintf(stderr, "compositeur GPU: init a echoue\n");
        return 1;
    }

    const struct {
        const char* name;
        BlendMode mode;
        float gains[3];
    } cases[] = {
        {"normal", BlendMode::Normal, {1.0f, 0.7f, 0.5f}},
        {"add", BlendMode::Add, {0.9f, 0.6f, 0.8f}},
        {"multiply", BlendMode::Multiply, {1.0f, 0.4f, 1.0f}},
        {"screen", BlendMode::Screen, {0.8f, 0.5f, 0.65f}},
        {"alpha", BlendMode::Alpha, {1.0f, 1.0f, 0.9f}},
        {"gains nuls", BlendMode::Screen, {0.0f, 0.0f, 0.0f}},
    };

    int failures = 0;
    for (const auto& test : cases) {
        // The CPU reference, exactly as the app composed before the GPU did.
        std::vector<std::uint8_t> expected;
        clear_program(expected, kSize, kSize);
        accumulate_layer(expected, kSize, kSize,
                         ComposeLayer{layer_a.data(), kSize, kSize, test.gains[0],
                                      BlendMode::Normal});
        accumulate_layer(expected, kSize, kSize,
                         ComposeLayer{layer_b.data(), kSize, kSize, test.gains[1],
                                      BlendMode::Add});
        accumulate_layer(expected, kSize, kSize,
                         ComposeLayer{layer_o.data(), kSize, kSize, test.gains[2],
                                      test.mode});

        gpu.render(tex_a, tex_b, tex_o, test.gains[0], test.gains[1], test.gains[2],
                   static_cast<int>(test.mode));
        // The readback is queued separately from render() so the app can read
        // the picture the EFFECT RACK produced rather than the compositor's own
        // target. There is no rack here, so the compositor's target IS the
        // program -- but the call has to be made, and forgetting it is what let
        // this tool keep passing from a stale binary while it could no longer
        // read anything at all.
        gpu.queue_readback(gpu.texture_index());

        const std::uint8_t* pixels = nullptr;
        for (int i = 0; i < 8 && pixels == nullptr; ++i) {
            pixels = gpu.completed_frame(bgfx::frame());
        }
        if (pixels == nullptr) {
            std::fprintf(stderr, "%-10s : aucun readback\n", test.name);
            ++failures;
            continue;
        }

        const int worst = worst_difference(pixels, expected);
        std::printf("%-10s : ecart max %d\n", test.name, worst);
        if (worst > 2) ++failures;
    }

    gpu.destroy();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d cas hors tolerance : le shader a quitte sa reference\n",
                     failures);
        return 2;
    }
    std::printf("compositeur GPU conforme a core/compose\n");
    return 0;
}
