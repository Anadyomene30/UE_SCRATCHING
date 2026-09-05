#include "core/compose.h"

#include <algorithm>
#include <cstring>

namespace svj {
namespace {

// The blend equations, in normalised space. One place, one truth: the fragment
// shader that eventually replaces this will be validated against these tests,
// so an equation changed here is an equation changed everywhere.
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

}  // namespace svj
