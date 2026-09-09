// scratchvj — holds the 360 shader to core/sphere.
//
// The equirect fed to the GPU encodes, in its own pixels, the direction each
// texel names: rgb = direction * 0.5 + 0.5, computed by the CPU reference.
// The expected output of the view pass is then ANALYTIC -- the colour of
// view_direction(view, pixel) -- with no CPU rasteriser to also get wrong.
// Encoding by direction rather than by uv makes the picture continuous across
// the wrap seam and at the poles, so a texel of sampling slack never explodes
// into a wrong-side-of-the-seam colour difference.
//
// Tolerance: the equirect discretises directions (one texel ~ 0.006 rad at this
// size, under 1 colour step) and the target rounds to 8 bits; 6/255 covers that
// with margin. A sign error, an axis swap, a flipped v -- the faults that
// matter -- disagree by tens to hundreds.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/sphere.h"
#include "gpu_view360.h"

using namespace svj;

namespace {

constexpr std::uint32_t kEquirectW = 1024;
constexpr std::uint32_t kEquirectH = 512;
constexpr std::uint32_t kViewW = 160;
constexpr std::uint32_t kViewH = 90;

std::uint8_t to_byte(double v) {
    const double scaled = (v * 0.5 + 0.5) * 255.0 + 0.5;
    return static_cast<std::uint8_t>(scaled < 0.0 ? 0.0 : (scaled > 255.0 ? 255.0 : scaled));
}

}  // namespace

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("sphere_check", 128, 128, SDL_WINDOW_HIDDEN);
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

    // The direction-coloured equirect, computed by the reference itself.
    std::vector<std::uint8_t> equirect(kEquirectW * kEquirectH * 4);
    for (std::uint32_t y = 0; y < kEquirectH; ++y) {
        for (std::uint32_t x = 0; x < kEquirectW; ++x) {
            const Vec2 uv{(x + 0.5) / kEquirectW, (y + 0.5) / kEquirectH};
            const Vec3 direction = direction_from_equirect(uv);
            std::uint8_t* p = equirect.data() + (y * kEquirectW + x) * 4;
            p[0] = to_byte(direction.x);
            p[1] = to_byte(direction.y);
            p[2] = to_byte(direction.z);
            p[3] = 255;
        }
    }
    const std::uint16_t equirect_tex =
        bgfx::createTexture2D(kEquirectW, kEquirectH, false, 1, bgfx::TextureFormat::RGBA8,
                              BGFX_SAMPLER_UVW_CLAMP,
                              bgfx::copy(equirect.data(),
                                         static_cast<std::uint32_t>(equirect.size())))
            .idx;

    svj::ui::View360Gpu pass;
    if (!pass.init(kViewW, kViewH, 0)) {
        std::fprintf(stderr, "passe 360: init a echoue\n");
        return 1;
    }

    // Readback plumbing for the tool only; the app never reads this pass back.
    bgfx::TextureHandle readback = bgfx::createTexture2D(
        kViewW, kViewH, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    std::vector<std::uint8_t> pixels(kViewW * kViewH * 4);

    SphereView cases[4];
    cases[0].projection = Projection::Perspective;  // dead ahead, the identity case
    cases[1].projection = Projection::Perspective;
    cases[1].yaw_deg = 35.0;
    cases[1].pitch_deg = -20.0;
    cases[1].roll_deg = 10.0;
    cases[1].fov_deg = 100.0;
    cases[2].projection = Projection::LittlePlanet;
    cases[2].yaw_deg = 120.0;
    cases[2].planet_zoom = 0.8;
    cases[3].projection = Projection::Fisheye;
    cases[3].pitch_deg = 30.0;
    cases[3].planet_zoom = 1.2;
    const char* names[4] = {"perspective 0", "perspective tourne", "little planet",
                            "fisheye"};

    int failures = 0;
    for (int c = 0; c < 4; ++c) {
        cases[c].aspect = static_cast<double>(kViewW) / kViewH;
        pass.render(equirect_tex, cases[c]);

        bgfx::TextureRegion dst;
        dst.init(readback, 0, 0);
        bgfx::TextureRegion src;
        {
            bgfx::TextureHandle target;
            target.idx = pass.texture_index();
            src.init(target, 0, 0);
        }
        bgfx::blit(1, dst, src);
        bgfx::TextureRegion whole;
        whole.init(readback, 0, 0);
        const std::uint32_t ready = bgfx::read(whole, pixels.data());
        while (bgfx::frame() < ready) {
        }

        int worst = 0;
        double sum = 0.0;
        for (std::uint32_t y = 0; y < kViewH; ++y) {
            for (std::uint32_t x = 0; x < kViewW; ++x) {
                const Vec2 uv{(x + 0.5) / kViewW, (y + 0.5) / kViewH};
                const Vec3 direction = view_direction(cases[c], uv);
                const std::uint8_t expected[3] = {to_byte(direction.x),
                                                  to_byte(direction.y),
                                                  to_byte(direction.z)};
                const std::uint8_t* got = pixels.data() + (y * kViewW + x) * 4;
                for (int k = 0; k < 3; ++k) {
                    const int d = std::abs(static_cast<int>(got[k]) -
                                           static_cast<int>(expected[k]));
                    if (d > worst) worst = d;
                    sum += d;
                }
            }
        }
        const double mean = sum / (static_cast<double>(kViewW) * kViewH * 3.0);
        std::printf("%-20s : ecart max %d, moyen %.2f\n", names[c], worst, mean);
        if (worst > 6 || mean > 1.5) ++failures;
    }

    pass.destroy();
    bgfx::destroy(readback);
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d projections hors tolerance : le shader a quitte core/sphere\n",
                     failures);
        return 2;
    }
    std::printf("reprojection 360 GPU conforme a core/sphere\n");
    return 0;
}
