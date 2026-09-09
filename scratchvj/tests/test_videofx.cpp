#include <algorithm>
#include <cmath>
#include <vector>

#include "core/videofx.h"
#include "harness.h"

using namespace svj;

namespace {

constexpr std::uint32_t kW = 32;
constexpr std::uint32_t kH = 24;

std::vector<std::uint8_t> checkerboard(int cell) {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(kW) * kH * 4);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            const bool on = ((x / cell) + (y / cell)) % 2 == 0;
            std::uint8_t* p = image.data() + (static_cast<std::size_t>(y) * kW + x) * 4;
            p[0] = p[1] = p[2] = on ? 255 : 0;
            p[3] = 255;
        }
    }
    return image;
}

std::vector<std::uint8_t> flat(std::uint8_t value) {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(kW) * kH * 4, 255);
    for (std::size_t i = 0; i < image.size(); i += 4) {
        image[i] = image[i + 1] = image[i + 2] = value;
    }
    return image;
}

// How far the picture strays from flat: the spread of its luminance. A low pass
// must lower it, a high pass must raise it.
double contrast(const std::vector<std::uint8_t>& image) {
    double mean = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < image.size(); i += 4) {
        mean += image[i];
        ++count;
    }
    mean /= count;
    double variance = 0.0;
    for (std::size_t i = 0; i < image.size(); i += 4) {
        variance += (image[i] - mean) * (image[i] - mean);
    }
    return std::sqrt(variance / count);
}

std::uint8_t red_at(const std::vector<std::uint8_t>& image, std::uint32_t x,
                    std::uint32_t y) {
    return image[(static_cast<std::size_t>(y) * kW + x) * 4];
}

}  // namespace

SVJ_TEST("videofx: an idle slot copies the picture through untouched") {
    // Every slot of the rack runs every frame; a slot at zero mix must cost the
    // picture nothing at all, not merely nothing visible.
    const auto source = checkerboard(4);
    std::vector<std::uint8_t> out;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::LowPass, 0.0f, 0.9f}, out);
    CHECK(out == source);
}

SVJ_TEST("videofx: THE LOW PASS REALLY REMOVES HIGH SPATIAL FREQUENCIES") {
    // The pairing the whole rack is built on: a low-pass filter in the time
    // domain IS a blur in the spatial one. So the test is the one you would run
    // on the audio side -- put in something with nothing but high frequencies,
    // and check it comes out flatter.
    const auto fine = checkerboard(1);   // the highest frequency the grid holds
    const auto coarse = checkerboard(8);

    std::vector<std::uint8_t> blurred_fine, blurred_coarse;
    apply_video_fx(fine.data(), kW, kH, VideoFx{EffectType::LowPass, 1.0f, 0.2f},
                   blurred_fine);
    apply_video_fx(coarse.data(), kW, kH, VideoFx{EffectType::LowPass, 1.0f, 0.2f},
                   blurred_coarse);

    // The fine pattern is nearly annihilated; the coarse one mostly survives.
    CHECK(contrast(blurred_fine) < contrast(fine) * 0.1);
    CHECK(contrast(blurred_coarse) > contrast(blurred_fine) * 3.0);
}

SVJ_TEST("videofx: a flat picture is unchanged by any amount of blur") {
    // The kernel must be normalised. An unnormalised one darkens or brightens
    // flat areas, which reads as the whole picture dimming when a knob moves.
    const auto source = flat(140);
    for (const float amount : {0.0f, 0.5f, 1.0f}) {
        std::vector<std::uint8_t> out;
        apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::LowPass, 1.0f, amount},
                       out);
        for (std::size_t i = 0; i < out.size(); i += 4) {
            CHECK_NEAR(out[i], 140, 1.0);
        }
    }
}

SVJ_TEST("videofx: the high pass is the picture minus its own low pass") {
    // Stated as a relation rather than as a look, because that relation is what
    // makes the pairing honest: it is the same subtraction the audio side does.
    const auto source = checkerboard(3);
    std::vector<std::uint8_t> low, high;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::LowPass, 1.0f, 0.3f}, low);
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::HighPass, 1.0f, 0.3f}, high);

    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            const double expected = static_cast<double>(red_at(source, x, y)) -
                                    red_at(low, x, y) + 127.5;
            CHECK_NEAR(red_at(high, x, y), std::clamp(expected, 0.0, 255.0), 2.0);
        }
    }
}

SVJ_TEST("videofx: posterisation quantises to the level count it promises") {
    // A gradient in, N distinct values out. The knob runs backwards -- more
    // amount is more crushing -- which is what the bitcrusher it pairs with does
    // to the sound, and getting that sense wrong would make the pair contradict.
    std::vector<std::uint8_t> gradient(static_cast<std::size_t>(kW) * kH * 4, 255);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            std::uint8_t* p = gradient.data() + (static_cast<std::size_t>(y) * kW + x) * 4;
            p[0] = p[1] = p[2] = static_cast<std::uint8_t>(x * 255 / (kW - 1));
        }
    }

    std::vector<std::uint8_t> out;
    apply_video_fx(gradient.data(), kW, kH, VideoFx{EffectType::Bitcrusher, 1.0f, 1.0f},
                   out);

    bool seen[256] = {};
    int distinct = 0;
    for (std::size_t i = 0; i < out.size(); i += 4) {
        if (!seen[out[i]]) {
            seen[out[i]] = true;
            ++distinct;
        }
    }
    CHECK_EQ(distinct, posterise_levels(1.0f));
    CHECK(posterise_levels(1.0f) < posterise_levels(0.0f));  // the knob's sense
}

SVJ_TEST("videofx: inverting twice returns the original") {
    const auto source = checkerboard(5);
    std::vector<std::uint8_t> once, twice;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::Invert, 1.0f, 0.5f}, once);
    apply_video_fx(once.data(), kW, kH, VideoFx{EffectType::Invert, 1.0f, 0.5f}, twice);
    CHECK(twice == source);
}

SVJ_TEST("videofx: the mirror is symmetric about the centre column") {
    const auto source = checkerboard(3);
    std::vector<std::uint8_t> out;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::Mirror, 1.0f, 0.5f}, out);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            CHECK_EQ(red_at(out, x, y), red_at(out, kW - 1 - x, y));
        }
    }
}

SVJ_TEST("videofx: a half-mixed effect lands halfway, not all or nothing") {
    // The mix knob is the one control every slot shares, so it has to be a real
    // blend rather than a threshold.
    const auto source = flat(0);
    std::vector<std::uint8_t> half;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::Invert, 0.5f, 0.5f}, half);
    for (std::size_t i = 0; i < half.size(); i += 4) CHECK_NEAR(half[i], 128, 1.0);
}

SVJ_TEST("videofx: an effect needing several frames is declared, not faked") {
    // The rack holds effects this file cannot do alone: a delay's trails read
    // the clip at several positions. Saying so lets the front end show nothing
    // rather than quietly drawing a dry frame and calling it an effect.
    CHECK(is_single_frame_effect(EffectType::LowPass));
    CHECK(!is_single_frame_effect(EffectType::Delay));
    CHECK(!is_single_frame_effect(EffectType::SlitScan));

    const auto source = checkerboard(4);
    std::vector<std::uint8_t> out;
    apply_video_fx(source.data(), kW, kH, VideoFx{EffectType::Delay, 1.0f, 0.5f}, out);
    CHECK(out == source);  // passed through, not approximated
}
