#include "core/videofx.h"

#include <algorithm>
#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;

// The gaussian kernel is 9 taps a side, always. The RADIUS grows with `amount`
// by stepping further between taps rather than by taking more of them, so the
// cost is fixed and the shader's loop bounds are a constant.
constexpr int kBlurTaps = 4;  // -4..+4

struct Rgb {
    double r = 0.0, g = 0.0, b = 0.0;
};

Rgb fetch(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height, int x,
          int y) {
    const int cx = std::clamp(x, 0, static_cast<int>(width) - 1);
    const int cy = std::clamp(y, 0, static_cast<int>(height) - 1);
    const std::uint8_t* p =
        rgba + (static_cast<std::size_t>(cy) * width + cx) * 4;
    return Rgb{p[0] / 255.0, p[1] / 255.0, p[2] / 255.0};
}

std::uint8_t to_byte(double v) {
    const double scaled = std::clamp(v, 0.0, 1.0) * 255.0 + 0.5;
    return static_cast<std::uint8_t>(scaled);
}

// The same weights the shader uses: a gaussian whose sigma is half the tap
// count, normalised over the 9x9 grid.
double gaussian_weight(int i, int j) {
    const double sigma = kBlurTaps * 0.5;
    return std::exp(-(i * i + j * j) / (2.0 * sigma * sigma));
}

Rgb blur_at(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height, int x,
            int y, int step) {
    Rgb sum;
    double total = 0.0;
    for (int j = -kBlurTaps; j <= kBlurTaps; ++j) {
        for (int i = -kBlurTaps; i <= kBlurTaps; ++i) {
            const double w = gaussian_weight(i, j);
            const Rgb s = fetch(rgba, width, height, x + i * step, y + j * step);
            sum.r += s.r * w;
            sum.g += s.g * w;
            sum.b += s.b * w;
            total += w;
        }
    }
    return Rgb{sum.r / total, sum.g / total, sum.b / total};
}

}  // namespace

bool is_single_frame_effect(EffectType type) {
    switch (type) {
        case EffectType::LowPass:
        case EffectType::HighPass:
        case EffectType::Bitcrusher:
        case EffectType::Invert:
        case EffectType::Mirror:
        case EffectType::Kaleidoscope:
            return true;
        default:
            return false;
    }
}

int blur_step_texels(float amount) {
    // One texel at the bottom of the knob, twelve at the top. Whole texels
    // because the shader samples with point filtering: a fractional step would
    // land wherever the hardware rounded it and the reference could not follow.
    return std::max(1, static_cast<int>(std::lround(1.0f + amount * 11.0f)));
}

int posterise_levels(float amount) {
    // Backwards on purpose: more `amount` is MORE crushing, so fewer levels.
    // A bitcrusher's knob does the same to the sound it is paired with.
    return std::max(2, static_cast<int>(std::lround(32.0f - amount * 30.0f)));
}

int kaleidoscope_segments(float amount) {
    return std::max(2, static_cast<int>(std::lround(2.0f + amount * 10.0f)));
}

void apply_video_fx(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                    const VideoFx& fx, std::vector<std::uint8_t>& out) {
    const std::size_t bytes = static_cast<std::size_t>(width) * height * 4;
    out.resize(bytes);
    if (rgba == nullptr || width == 0 || height == 0) return;

    const double mix = std::clamp(fx.mix, 0.0f, 1.0f);
    if (mix <= 0.0 || !is_single_frame_effect(fx.type)) {
        std::copy(rgba, rgba + bytes, out.begin());
        return;
    }

    const int step = blur_step_texels(fx.amount);
    const int levels = posterise_levels(fx.amount);
    const int segments = kaleidoscope_segments(fx.amount);

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const Rgb dry = fetch(rgba, width, height, static_cast<int>(x),
                                  static_cast<int>(y));
            Rgb wet = dry;

            switch (fx.type) {
                case EffectType::LowPass:
                    wet = blur_at(rgba, width, height, static_cast<int>(x),
                                  static_cast<int>(y), step);
                    break;

                case EffectType::HighPass: {
                    // Exactly what it is in the other domain: the signal minus
                    // its own low pass, lifted to mid grey so the result is
                    // something a screen can show.
                    const Rgb low = blur_at(rgba, width, height, static_cast<int>(x),
                                            static_cast<int>(y), step);
                    wet = Rgb{dry.r - low.r + 0.5, dry.g - low.g + 0.5,
                              dry.b - low.b + 0.5};
                    break;
                }

                case EffectType::Bitcrusher: {
                    const double n = levels - 1;
                    wet = Rgb{std::round(dry.r * n) / n, std::round(dry.g * n) / n,
                              std::round(dry.b * n) / n};
                    break;
                }

                case EffectType::Invert:
                    wet = Rgb{1.0 - dry.r, 1.0 - dry.g, 1.0 - dry.b};
                    break;

                case EffectType::Mirror: {
                    // The left half, reflected. Reading from the left rather
                    // than folding in place means the seam falls exactly on the
                    // centre column whatever the width's parity.
                    const int mirrored =
                        x < width / 2 ? static_cast<int>(x)
                                      : static_cast<int>(width) - 1 - static_cast<int>(x);
                    wet = fetch(rgba, width, height, mirrored, static_cast<int>(y));
                    break;
                }

                case EffectType::Kaleidoscope: {
                    // Polar fold: every wedge shows the same one, mirrored
                    // alternately so the seams meet instead of cutting.
                    const double cx = (x + 0.5) / width - 0.5;
                    const double cy = (y + 0.5) / height - 0.5;
                    const double radius = std::sqrt(cx * cx + cy * cy);
                    double angle = std::atan2(cy, cx);
                    const double wedge = 2.0 * kPi / segments;
                    angle = std::fmod(angle + 2.0 * kPi, wedge);
                    if (angle > wedge * 0.5) angle = wedge - angle;
                    const double su = 0.5 + std::cos(angle) * radius;
                    const double sv = 0.5 + std::sin(angle) * radius;
                    wet = fetch(rgba, width, height,
                                static_cast<int>(su * width),
                                static_cast<int>(sv * height));
                    break;
                }

                default:
                    break;
            }

            std::uint8_t* p = out.data() + (static_cast<std::size_t>(y) * width + x) * 4;
            p[0] = to_byte(dry.r + (wet.r - dry.r) * mix);
            p[1] = to_byte(dry.g + (wet.g - dry.g) * mix);
            p[2] = to_byte(dry.b + (wet.b - dry.b) * mix);
            p[3] = 255;
        }
    }
}

}  // namespace svj
