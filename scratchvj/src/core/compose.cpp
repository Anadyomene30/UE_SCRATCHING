#include "core/compose.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace svj {
namespace {

// The blend equations, in normalised space. One place, one truth: the fragment
// shader that mirrors this is validated against these tests, so an equation
// changed here is an equation changed everywhere.
float blend_channel(BlendMode mode, float under, float over) {
    switch (mode) {
        case BlendMode::Add:
            return std::min(1.0f, under + over);
        case BlendMode::Multiply:
            return under * over;
        case BlendMode::Screen:
            return 1.0f - (1.0f - under) * (1.0f - over);
        case BlendMode::Normal:
        case BlendMode::Alpha:  // per-channel part; the alpha weighting is below
        default:
            return over;
    }
}

float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

float mix(float a, float b, float t) { return a + (b - a) * t; }

// GLSL's smoothstep, so the shader and this agree to the rounding.
float smoothstep(float edge0, float edge1, float x) {
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

// Rec. 601 luma, the one the wipe reads. Not a colour-science claim: a
// threshold on brightness that both paths compute identically.
float luma(const float rgb[3]) { return 0.299f * rgb[0] + 0.587f * rgb[1] + 0.114f * rgb[2]; }

// Nearest-neighbour fetch of a layer at (u, v) in 0..1, into `out` (0..1).
// A null layer is black.
void fetch(const ComposeLayer& layer, float u, float v, float out[4]) {
    if (layer.rgba == nullptr || layer.width == 0 || layer.height == 0) {
        out[0] = out[1] = out[2] = 0.0f;
        out[3] = 1.0f;
        return;
    }
    const float cu = clamp01(u);
    const float cv = clamp01(v);
    std::uint32_t sx = static_cast<std::uint32_t>(cu * static_cast<float>(layer.width));
    std::uint32_t sy = static_cast<std::uint32_t>(cv * static_cast<float>(layer.height));
    sx = std::min(sx, layer.width - 1);
    sy = std::min(sy, layer.height - 1);
    const std::uint8_t* p = layer.rgba + (static_cast<std::size_t>(sy) * layer.width + sx) * 4;
    for (int c = 0; c < 4; ++c) out[c] = static_cast<float>(p[c]) / 255.0f;
}

}  // namespace

void clear_program(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                   std::uint32_t height) {
    canvas.assign(static_cast<std::size_t>(width) * height * 4, 0);
    for (std::size_t i = 3; i < canvas.size(); i += 4) canvas[i] = 255;
}

void accumulate_layer(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                      std::uint32_t height, const ComposeLayer& layer) {
    if (layer.rgba == nullptr || layer.gain <= 0.0f) return;
    if (layer.width == 0 || layer.height == 0 || width == 0 || height == 0) return;
    if (canvas.size() < static_cast<std::size_t>(width) * height * 4) return;

    const float gain = std::min(layer.gain, 1.0f);

    for (std::uint32_t y = 0; y < height; ++y) {
        // Nearest-neighbour source row for this output row.
        const std::uint32_t sy =
            static_cast<std::uint32_t>((static_cast<std::uint64_t>(y) * layer.height) /
                                       height);
        const std::uint8_t* src_row =
            layer.rgba + static_cast<std::size_t>(sy) * layer.width * 4;
        std::uint8_t* dst_row = canvas.data() + static_cast<std::size_t>(y) * width * 4;

        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t sx =
                static_cast<std::uint32_t>((static_cast<std::uint64_t>(x) * layer.width) /
                                           width);
            const std::uint8_t* src = src_row + static_cast<std::size_t>(sx) * 4;
            std::uint8_t* dst = dst_row + static_cast<std::size_t>(x) * 4;

            // Alpha mode reads the source's own alpha on top of the gain; every
            // other mode treats the source as opaque, which is what BC1 decodes
            // to anyway.
            float weight = gain;
            if (layer.blend == BlendMode::Alpha) {
                weight *= static_cast<float>(src[3]) / 255.0f;
            }

            for (int c = 0; c < 3; ++c) {
                const float under = static_cast<float>(dst[c]) / 255.0f;
                const float over = static_cast<float>(src[c]) / 255.0f;
                const float blended = blend_channel(layer.blend, under, over);
                const float mixed = under + (blended - under) * weight;
                dst[c] = static_cast<std::uint8_t>(mixed * 255.0f + 0.5f);
            }
            dst[3] = 255;
        }
    }
}

void compose_decks(std::vector<std::uint8_t>& canvas, std::uint32_t width,
                   std::uint32_t height, const ComposeLayer& deck_a,
                   const ComposeLayer& deck_b, const Crossfade& crossfade) {
    if (width == 0 || height == 0) return;
    if (canvas.size() < static_cast<std::size_t>(width) * height * 4) return;

    const float ga = clamp01(crossfade.gain_a);
    const float gb = clamp01(crossfade.gain_b);
    const float p = clamp01(crossfade.position);

    for (std::uint32_t y = 0; y < height; ++y) {
        // Pixel centres, so the shader's v_texcoord0 and this land on the
        // same texel for the same output pixel.
        const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
        std::uint8_t* dst_row = canvas.data() + static_cast<std::size_t>(y) * width * 4;
        for (std::uint32_t x = 0; x < width; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            float a[4], b[4];
            fetch(deck_a, u, v, a);
            fetch(deck_b, u, v, b);
            // The weighted decks: the curves and channel faders have spoken.
            float wa[3], wb[3], c[3];
            for (int k = 0; k < 3; ++k) {
                wa[k] = a[k] * ga;
                wb[k] = b[k] * gb;
            }

            switch (crossfade.transition) {
                case Transition::Cut:
                    for (int k = 0; k < 3; ++k) c[k] = p < 0.5f ? wa[k] : wb[k];
                    break;
                case Transition::Fade:
                    // B, Normal over A, eased in by its weight.
                    for (int k = 0; k < 3; ++k) c[k] = mix(wa[k], b[k], gb);
                    break;
                case Transition::Multiply:
                case Transition::Screen: {
                    // A, then the blend of both, then B: the blend is what the
                    // middle of the travel shows.
                    for (int k = 0; k < 3; ++k) {
                        const float blended =
                            crossfade.transition == Transition::Multiply
                                ? wa[k] * wb[k]
                                : 1.0f - (1.0f - wa[k]) * (1.0f - wb[k]);
                        c[k] = p < 0.5f ? mix(wa[k], blended, p * 2.0f)
                                        : mix(blended, wb[k], p * 2.0f - 1.0f);
                    }
                    break;
                }
                case Transition::LumaWipe: {
                    // A's dark pixels give way first. The threshold runs from
                    // below zero to above one so the ends are whole pictures.
                    const float t = p * 1.16f - 0.08f;
                    const float k = smoothstep(t - 0.08f, t + 0.08f, luma(wa));
                    for (int i = 0; i < 3; ++i) c[i] = mix(wb[i], wa[i], k);
                    break;
                }
                case Transition::GeoWipe: {
                    // Left to right, with a soft edge.
                    const float t = p * 1.06f - 0.03f;
                    const float k = smoothstep(t - 0.03f, t + 0.03f, u);
                    for (int i = 0; i < 3; ++i) c[i] = mix(wb[i], wa[i], k);
                    break;
                }
                case Transition::RgbSplit:
                    // The channels cross one after the other: red first.
                    for (int k = 0; k < 3; ++k) {
                        c[k] = mix(wa[k], wb[k], clamp01(p * 3.0f - static_cast<float>(k)));
                    }
                    break;
                case Transition::ZoomBlur: {
                    // A zooms in as it leaves, B fades up underneath. Not a
                    // blur -- the taps that would blur it are the effect
                    // rack's business -- and named so on screen.
                    const float zoom = 1.0f + p * 1.5f;
                    float az[4];
                    fetch(deck_a, 0.5f + (u - 0.5f) / zoom, 0.5f + (v - 0.5f) / zoom, az);
                    for (int k = 0; k < 3; ++k) c[k] = mix(az[k] * ga, wb[k], p);
                    break;
                }
                case Transition::Additive:
                default:
                    // B added to A, eased in by its weight: the constant-power
                    // crossfade a scratch never passes through black on.
                    for (int k = 0; k < 3; ++k) {
                        c[k] = mix(wa[k], std::min(1.0f, wa[k] + b[k]), gb);
                    }
                    break;
            }

            std::uint8_t* dst = dst_row + static_cast<std::size_t>(x) * 4;
            for (int k = 0; k < 3; ++k) dst[k] = static_cast<std::uint8_t>(clamp01(c[k]) * 255.0f + 0.5f);
            dst[3] = 255;
        }
    }
}

Transition transition_from_unit(float value01) {
    const int step = static_cast<int>(clamp01(value01) * 8.999f);
    return static_cast<Transition>(step);
}

const char* transition_name(Transition transition) {
    switch (transition) {
        case Transition::Cut: return "cut";
        case Transition::Fade: return "fade";
        case Transition::Additive: return "additive";
        case Transition::Multiply: return "multiply";
        case Transition::Screen: return "screen";
        case Transition::LumaWipe: return "luma_wipe";
        case Transition::GeoWipe: return "geo_wipe";
        case Transition::RgbSplit: return "rgb_split";
        case Transition::ZoomBlur: return "zoom";
    }
    return "additive";
}

}  // namespace svj
