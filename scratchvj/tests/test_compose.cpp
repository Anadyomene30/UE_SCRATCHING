#include <cstdint>
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
