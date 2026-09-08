#include <cstdint>
#include <string>
#include <vector>

#include "core/compose.h"
#include "harness.h"

using namespace svj;

namespace {

std::vector<std::uint8_t> solid(std::uint32_t w, std::uint32_t h, std::uint8_t r,
                                std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    std::vector<std::uint8_t> image(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < image.size(); i += 4) {
        image[i] = r;
        image[i + 1] = g;
        image[i + 2] = b;
        image[i + 3] = a;
    }
    return image;
}

ComposeLayer layer_of(const std::vector<std::uint8_t>& image, std::uint32_t w,
                      std::uint32_t h, float gain, BlendMode mode) {
    ComposeLayer layer;
    layer.rgba = image.data();
    layer.width = w;
    layer.height = h;
    layer.gain = gain;
    layer.blend = mode;
    return layer;
}

}  // namespace

SVJ_TEST("compose: the program starts opaque black, not undefined") {
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    CHECK_EQ(canvas.size(), std::size_t{4 * 4 * 4});
    CHECK_EQ(canvas[0], 0);
    CHECK_EQ(canvas[3], 255);
}

SVJ_TEST("compose: a full-gain normal layer replaces what is under it") {
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 8, 8);
    const auto red = solid(8, 8, 200, 10, 30);
    accumulate_layer(canvas, 8, 8, layer_of(red, 8, 8, 1.0f, BlendMode::Normal));
    CHECK_EQ(canvas[0], 200);
    CHECK_EQ(canvas[1], 10);
    CHECK_EQ(canvas[2], 30);
}

SVJ_TEST("compose: gain fades a layer in, and zero gain is a strict no-op") {
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    const auto white = solid(4, 4, 255, 255, 255);

    accumulate_layer(canvas, 4, 4, layer_of(white, 4, 4, 0.0f, BlendMode::Normal));
    CHECK_EQ(canvas[0], 0);  // untouched

    accumulate_layer(canvas, 4, 4, layer_of(white, 4, 4, 0.5f, BlendMode::Normal));
    CHECK_NEAR(canvas[0], 128, 1.0);  // halfway there
}

SVJ_TEST("compose: each blend mode does its own arithmetic") {
    // 0.5 over 0.5: add gives 1.0, multiply 0.25, screen 0.75. The same three
    // numbers the fragment shader will be tested against later.
    const auto base = solid(2, 2, 128, 128, 128);
    const auto over = solid(2, 2, 128, 128, 128);

    const auto run = [&](BlendMode mode) {
        std::vector<std::uint8_t> canvas;
        clear_program(canvas, 2, 2);
        accumulate_layer(canvas, 2, 2, layer_of(base, 2, 2, 1.0f, BlendMode::Normal));
        accumulate_layer(canvas, 2, 2, layer_of(over, 2, 2, 1.0f, mode));
        return static_cast<int>(canvas[0]);
    };

    CHECK_NEAR(run(BlendMode::Add), 255, 1.0);
    CHECK_NEAR(run(BlendMode::Multiply), 64, 1.0);
    CHECK_NEAR(run(BlendMode::Screen), 191, 1.0);
}

SVJ_TEST("compose: alpha mode reads the source's own alpha") {
    // A half-transparent white overlay over black lands mid-grey; the logo layer
    // depends on exactly this.
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 2, 2);
    const auto veil = solid(2, 2, 255, 255, 255, 128);
    accumulate_layer(canvas, 2, 2, layer_of(veil, 2, 2, 1.0f, BlendMode::Alpha));
    CHECK_NEAR(canvas[0], 128, 2.0);
}

SVJ_TEST("compose: a source of a different size is stretched over the canvas") {
    // Left half red, right half blue, in a 2x1 source onto an 8x4 canvas: the
    // split must land in the middle, which is what nearest-neighbour owes us.
    std::vector<std::uint8_t> source = {255, 0, 0, 255, 0, 0, 255, 255};
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 8, 4);

    ComposeLayer layer;
    layer.rgba = source.data();
    layer.width = 2;
    layer.height = 1;
    layer.gain = 1.0f;
    layer.blend = BlendMode::Normal;
    accumulate_layer(canvas, 8, 4, layer);

    CHECK_EQ(canvas[(0 * 8 + 1) * 4 + 0], 255);  // left: red
    CHECK_EQ(canvas[(0 * 8 + 1) * 4 + 2], 0);
    CHECK_EQ(canvas[(3 * 8 + 6) * 4 + 0], 0);  // right: blue
    CHECK_EQ(canvas[(3 * 8 + 6) * 4 + 2], 255);
}

SVJ_TEST("compose: a null source or an empty canvas is refused quietly") {
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    ComposeLayer empty;
    accumulate_layer(canvas, 4, 4, empty);  // must not crash
    CHECK_EQ(canvas[0], 0);

    std::vector<std::uint8_t> tiny(3);  // far too small for 4x4
    const auto red = solid(4, 4, 255, 0, 0);
    accumulate_layer(tiny, 4, 4, layer_of(red, 4, 4, 1.0f, BlendMode::Normal));
    CHECK_EQ(tiny.size(), std::size_t{3});  // untouched, not overrun
}

namespace {

Crossfade at(Transition transition, float position, float gain_a = 1.0f, float gain_b = 1.0f) {
    Crossfade xf;
    xf.transition = transition;
    xf.position = position;
    xf.gain_a = gain_a;
    xf.gain_b = gain_b;
    return xf;
}

const Transition kAll[] = {Transition::Cut,      Transition::Fade,     Transition::Additive,
                           Transition::Multiply, Transition::Screen,   Transition::LumaWipe,
                           Transition::GeoWipe,  Transition::RgbSplit, Transition::ZoomBlur};

}  // namespace

SVJ_TEST("crossfade: EVERY TRANSITION SHOWS A ALONE AT 0 AND B ALONE AT 1") {
    // Whatever happens in between, the ends are the two pictures, whole. A
    // transition that did not honour that is a crossfader that never arrives.
    const auto red = solid(8, 8, 200, 40, 20);
    const auto blue = solid(8, 8, 10, 60, 220);
    for (const Transition t : kAll) {
        std::vector<std::uint8_t> canvas;
        clear_program(canvas, 8, 8);
        compose_decks(canvas, 8, 8, layer_of(red, 8, 8, 1.0f, BlendMode::Normal),
                      layer_of(blue, 8, 8, 1.0f, BlendMode::Normal), at(t, 0.0f, 1.0f, 0.0f));
        CHECK_EQ(canvas[0], 200);
        CHECK_EQ(canvas[2], 20);
        clear_program(canvas, 8, 8);
        compose_decks(canvas, 8, 8, layer_of(red, 8, 8, 1.0f, BlendMode::Normal),
                      layer_of(blue, 8, 8, 1.0f, BlendMode::Normal), at(t, 1.0f, 0.0f, 1.0f));
        CHECK_EQ(canvas[0], 10);
        CHECK_EQ(canvas[2], 220);
    }
}

SVJ_TEST("crossfade: additive is what the compositor always did") {
    const auto red = solid(4, 4, 200, 0, 0);
    const auto blue = solid(4, 4, 0, 0, 200);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    compose_decks(canvas, 4, 4, layer_of(red, 4, 4, 1.0f, BlendMode::Normal),
                  layer_of(blue, 4, 4, 1.0f, BlendMode::Normal), at(Transition::Additive, 0.5f, 1.0f, 1.0f));
    CHECK_EQ(canvas[0], 200);  // both fully present: a sharp curve's middle
    CHECK_EQ(canvas[2], 200);
}

SVJ_TEST("crossfade: multiply and screen show the blend at the middle of the travel") {
    const auto grey = solid(4, 4, 128, 128, 128);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    compose_decks(canvas, 4, 4, layer_of(grey, 4, 4, 1.0f, BlendMode::Normal),
                  layer_of(grey, 4, 4, 1.0f, BlendMode::Normal), at(Transition::Multiply, 0.5f));
    CHECK_NEAR(canvas[0], 64, 1.5);  // 0.5 * 0.5
    clear_program(canvas, 4, 4);
    compose_decks(canvas, 4, 4, layer_of(grey, 4, 4, 1.0f, BlendMode::Normal),
                  layer_of(grey, 4, 4, 1.0f, BlendMode::Normal), at(Transition::Screen, 0.5f));
    CHECK_NEAR(canvas[0], 191, 1.5);  // 1 - 0.5 * 0.5
}

SVJ_TEST("crossfade: the geometric wipe has crossed the left half at mid travel") {
    const auto red = solid(16, 4, 255, 0, 0);
    const auto blue = solid(16, 4, 0, 0, 255);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 16, 4);
    compose_decks(canvas, 16, 4, layer_of(red, 16, 4, 1.0f, BlendMode::Normal),
                  layer_of(blue, 16, 4, 1.0f, BlendMode::Normal), at(Transition::GeoWipe, 0.5f));
    CHECK_EQ(canvas[0 * 4 + 2], 255);   // leftmost pixel: B
    CHECK_EQ(canvas[15 * 4 + 0], 255);  // rightmost pixel: still A
}

SVJ_TEST("crossfade: the luma wipe takes A's dark pixels first") {
    std::vector<std::uint8_t> a(16 * 4 * 4);
    for (std::uint32_t x = 0; x < 16; ++x) {
        for (std::uint32_t y = 0; y < 4; ++y) {
            std::uint8_t* p = a.data() + (y * 16 + x) * 4;
            p[0] = p[1] = p[2] = x < 8 ? 20 : 240;  // dark left, bright right
            p[3] = 255;
        }
    }
    const auto blue = solid(16, 4, 0, 0, 255);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 16, 4);
    compose_decks(canvas, 16, 4, layer_of(a, 16, 4, 1.0f, BlendMode::Normal),
                  layer_of(blue, 16, 4, 1.0f, BlendMode::Normal), at(Transition::LumaWipe, 0.5f));
    CHECK_EQ(canvas[0 * 4 + 2], 255);   // dark A pixel: replaced by B
    CHECK_EQ(canvas[15 * 4 + 0], 240);  // bright A pixel: still A
}

SVJ_TEST("crossfade: the rgb split crosses red first and blue last") {
    const auto white = solid(4, 4, 255, 255, 255);
    const auto black = solid(4, 4, 0, 0, 0);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    compose_decks(canvas, 4, 4, layer_of(white, 4, 4, 1.0f, BlendMode::Normal),
                  layer_of(black, 4, 4, 1.0f, BlendMode::Normal), at(Transition::RgbSplit, 0.4f));
    CHECK_EQ(canvas[0], 0);     // red has fully crossed to B
    CHECK(canvas[1] > 0 && canvas[1] < 255);  // green mid-way
    CHECK_EQ(canvas[2], 255);   // blue not yet
}

SVJ_TEST("crossfade: the zoom shows the centre of A larger as it leaves") {
    // A: a dark picture with a bright centre pixel block. Zoomed in, the
    // corner of the output reads a pixel nearer the centre of A.
    std::vector<std::uint8_t> a(16 * 16 * 4, 0);
    for (std::uint32_t y = 6; y < 10; ++y) {
        for (std::uint32_t x = 6; x < 10; ++x) {
            std::uint8_t* p = a.data() + (y * 16 + x) * 4;
            p[0] = p[1] = p[2] = 255;
            p[3] = 255;
        }
    }
    const auto black = solid(16, 16, 0, 0, 0);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 16, 16);
    compose_decks(canvas, 16, 16, layer_of(a, 16, 16, 1.0f, BlendMode::Normal),
                  layer_of(black, 16, 16, 1.0f, BlendMode::Normal), at(Transition::ZoomBlur, 0.6f));
    // Zoom 1.9 at p = 0.6: output pixel 4 reads A at 0.5 + (4.5/16 - 0.5)/1.9 = 0.386 -> x 6: bright.
    CHECK(canvas[(4 * 16 + 4) * 4] > 60);
    CHECK_EQ(canvas[(0 * 16 + 0) * 4], 0);
}

SVJ_TEST("crossfade: a null deck is black, not a crash") {
    const auto red = solid(4, 4, 200, 0, 0);
    std::vector<std::uint8_t> canvas;
    clear_program(canvas, 4, 4);
    compose_decks(canvas, 4, 4, layer_of(red, 4, 4, 1.0f, BlendMode::Normal), ComposeLayer{},
                  at(Transition::Fade, 1.0f, 0.0f, 1.0f));
    CHECK_EQ(canvas[0], 0);
}

SVJ_TEST("crossfade: a control's travel lands on all nine transitions in order") {
    CHECK(transition_from_unit(0.0f) == Transition::Cut);
    CHECK(transition_from_unit(0.5f) == Transition::Screen);
    CHECK(transition_from_unit(1.0f) == Transition::ZoomBlur);
    CHECK_EQ(std::string(transition_name(Transition::LumaWipe)), std::string("luma_wipe"));
}
