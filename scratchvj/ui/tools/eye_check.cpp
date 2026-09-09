// scratchvj — holds the per-eye shader to core/headset.
//
// Same technique as sphere_check, and for the same reason: the equirect encodes
// in its own pixels the direction each texel names, so the expected output is
// ANALYTIC -- the colour of eye_direction(world, eye, pixel) -- with no CPU
// rasteriser to also get wrong.
//
// What this catches that no unit test can: a shader that drops the quaternion,
// that centres the ray instead of interpolating the four bounds, or that applies
// the performer's rotation before the head's instead of after. All three produce
// a picture that looks like 360 video and is wrong by tens of degrees -- the
// class of fault that is invisible until someone puts the headset on and feels
// sick.
//
// Tolerance is sphere_check's, for the same arithmetic: one equirect texel is
// well under a colour step at this size, and 6/255 covers that with margin while
// a sign error or an axis swap disagrees by tens.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/headset.h"
#include "core/sphere.h"
#include "gpu_eye.h"

using namespace svj;

namespace {

constexpr std::uint32_t kEquirectW = 1024;
constexpr std::uint32_t kEquirectH = 512;
constexpr std::uint32_t kViewW = 160;
constexpr std::uint32_t kViewH = 160;
constexpr double kPi = 3.14159265358979323846;

std::uint8_t to_byte(double v) {
    const double scaled = (v * 0.5 + 0.5) * 255.0 + 0.5;
    return static_cast<std::uint8_t>(scaled < 0.0 ? 0.0 : (scaled > 255.0 ? 255.0 : scaled));
}

double radians(double degrees) { return degrees * kPi / 180.0; }

// A yaw-then-pitch-then-roll pose, the way a head actually sits.
Quat pose(double yaw_deg, double pitch_deg, double roll_deg) {
    const double hy = radians(-yaw_deg) * 0.5;
    const double hp = radians(pitch_deg) * 0.5;
    const double hr = radians(roll_deg) * 0.5;
    const Quat y{0.0, std::sin(hy), 0.0, std::cos(hy)};
    const Quat p{std::sin(hp), 0.0, 0.0, std::cos(hp)};
    const Quat r{0.0, 0.0, std::sin(hr), std::cos(hr)};
    return concat(y, concat(p, r));
}

struct Case {
    const char* name;
    SphereView world;
    EyeView eye;
};

}  // namespace

int main() {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("eye_check", 128, 128, SDL_WINDOW_HIDDEN);
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

    svj::ui::EyeGpu pass;
    if (!pass.init(kViewW, kViewH, 0)) {
        std::fprintf(stderr, "passe oeil: init a echoue\n");
        return 1;
    }

    bgfx::TextureHandle readback = bgfx::createTexture2D(
        kViewW, kViewH, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    std::vector<std::uint8_t> pixels(kViewW * kViewH * 4);

    // A field of view shaped like a real headset lens: wider towards the nose
    // than away from it, and taller below than above.
    const FovTangents canted_fov =
        fov_from_angles(radians(-52.0), radians(43.0), radians(48.0), radians(-50.0));

    std::vector<Case> cases;
    {
        Case c{"tete immobile", SphereView{}, EyeView{}};
        c.eye.fov = fov_from_horizontal(90.0, 1.0);
        cases.push_back(c);
    }
    {
        Case c{"tete tournee", SphereView{}, EyeView{}};
        c.eye.fov = fov_from_horizontal(100.0, 1.0);
        c.eye.orientation = pose(40.0, -25.0, 12.0);
        cases.push_back(c);
    }
    {
        // The case a symmetric projection gets wrong and nothing else does.
        Case c{"fov asymetrique", SphereView{}, EyeView{}};
        c.eye.fov = canted_fov;
        cases.push_back(c);
    }
    {
        // The full composition: the performer spins the sphere while the head
        // turns inside it. Getting the ORDER wrong still produces a plausible
        // picture, which is exactly why it is checked.
        //
        // This case is the ONLY one that can catch it, and that was measured
        // rather than assumed: swapping the two rotations in the shader leaves
        // the three cases above at 1/255 and moves this one to 58/255. A case
        // with only one rotation cannot see an ordering fault at all.
        Case c{"monde + tete", SphereView{}, EyeView{}};
        c.world.yaw_deg = 65.0;
        c.world.pitch_deg = 15.0;
        c.world.roll_deg = -8.0;
        c.eye.fov = canted_fov;
        c.eye.orientation = pose(-30.0, 20.0, 5.0);
        cases.push_back(c);
    }

    int failures = 0;
    for (const Case& test : cases) {
        pass.render(equirect_tex, test.world, test.eye);

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
        for (std::uint32_t y = 0; y < kViewH; ++y) {
            for (std::uint32_t x = 0; x < kViewW; ++x) {
                const Vec2 uv{(x + 0.5) / kViewW, (y + 0.5) / kViewH};
                const Vec3 direction = eye_direction(test.world, test.eye, uv);
                const std::uint8_t expected[3] = {to_byte(direction.x),
                                                  to_byte(direction.y),
                                                  to_byte(direction.z)};
                const std::uint8_t* got = pixels.data() + (y * kViewW + x) * 4;
                for (int ch = 0; ch < 3; ++ch) {
                    const int difference = std::abs(got[ch] - expected[ch]);
                    if (difference > worst) worst = difference;
                }
            }
        }
        std::printf("%-18s : ecart max %d\n", test.name, worst);
        if (worst > 6) ++failures;
    }

    pass.destroy();
    bgfx::destroy(readback);
    bgfx::destroy(bgfx::TextureHandle{equirect_tex});
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failures > 0) {
        std::fprintf(stderr, "%d cas hors tolerance : la passe oeil a quitte sa reference\n",
                     failures);
        return 2;
    }
    std::printf("passe oeil conforme a core/headset\n");
    return 0;
}
