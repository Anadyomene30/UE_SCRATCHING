#include <cmath>
#include <string>

#include "config/warp_io.h"
#include "harness.h"

using namespace svj;

namespace {

double distance(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// A preset shaped like something a night on a ladder produces: an uneven grid
// with points pulled about, a pin off square, and a mask.
OutputPreset shaped() {
    OutputPreset preset;
    preset.name = "salle voutee";
    preset.pin.top_left = Point{0.04, 0.02};
    preset.pin.bottom_right = Point{0.97, 0.94};

    preset.mesh.reset(3, 3);
    preset.mesh.insert_column(1);   // makes the columns UNEVEN, which is the point
    preset.mesh.set_interpolation(WarpInterpolation::Bezier);
    preset.mesh.set(1, 1, Point{0.30, 0.44});
    preset.mesh.set(2, 2, Point{0.66, 0.71});
    preset.mesh_enabled = true;

    preset.mask = {Point{0.1, 0.1}, Point{0.9, 0.12}, Point{0.85, 0.9}};
    preset.mask_feather = 0.04;
    return preset;
}

}  // namespace

SVJ_TEST("preset: A SAVED MAPPING COMES BACK AIMED AT THE SAME WALL") {
    // The whole reason presets exist. Not "the fields round-trip" -- the SURFACE
    // has to, which is what a projector actually shows. Sampled densely, because
    // an error in the source spacing would leave every corner in place and still
    // redistribute the picture between them.
    const OutputPreset original = shaped();

    OutputPreset restored;
    std::string error;
    CHECK(preset_from_json(preset_to_json(original), restored, error));
    CHECK_EQ(error, std::string{});

    CHECK_EQ(restored.name, original.name);
    CHECK_EQ(restored.mesh.cols(), original.mesh.cols());
    CHECK_EQ(restored.mesh.rows(), original.mesh.rows());
    CHECK(restored.mesh_enabled);
    CHECK(restored.mesh.interpolation() == WarpInterpolation::Bezier);

    double worst = 0.0;
    for (int i = 0; i <= 40; ++i) {
        for (int j = 0; j <= 40; ++j) {
            const double u = i / 40.0;
            const double v = j / 40.0;
            worst = std::max(worst, distance(original.mesh.map(u, v),
                                             restored.mesh.map(u, v)));
        }
    }
    CHECK(worst < 1e-9);
}

SVJ_TEST("preset: the uneven source spacing survives the trip") {
    // The failure this guards against is silent and total: reconstruct the
    // column coordinates by dividing instead of reading them back, and every
    // corner lands right while the image between them slides.
    const OutputPreset original = shaped();
    OutputPreset restored;
    std::string error;
    CHECK(preset_from_json(preset_to_json(original), restored, error));

    for (int col = 0; col < original.mesh.cols(); ++col) {
        CHECK_NEAR(restored.mesh.column_u(col), original.mesh.column_u(col), 1e-12);
    }
    // And it really was uneven, or this test would prove nothing. Splitting one
    // cell of an even grid leaves the two halves equal to each OTHER, so the
    // comparison has to reach the cell that was never split -- the last one.
    const int last = original.mesh.cols() - 1;
    const double narrow = original.mesh.column_u(1) - original.mesh.column_u(0);
    const double wide = original.mesh.column_u(last) - original.mesh.column_u(last - 1);
    CHECK(std::fabs(narrow - wide) > 1e-6);
}

SVJ_TEST("preset: the pin and the mask round-trip") {
    const OutputPreset original = shaped();
    OutputPreset restored;
    std::string error;
    CHECK(preset_from_json(preset_to_json(original), restored, error));

    CHECK(distance(restored.pin.top_left, original.pin.top_left) < 1e-12);
    CHECK(distance(restored.pin.bottom_right, original.pin.bottom_right) < 1e-12);
    CHECK_EQ(restored.mask.size(), original.mask.size());
    CHECK_NEAR(restored.mask_feather, original.mask_feather, 1e-12);
    for (std::size_t i = 0; i < original.mask.size(); ++i) {
        CHECK(distance(restored.mask[i], original.mask[i]) < 1e-12);
    }
}

SVJ_TEST("preset: a corrupt file is refused with a reason, not half-loaded") {
    // A preset is read minutes before a set. Leaving the output half-configured
    // would be worse than refusing, so the destination is only written once the
    // whole document has parsed.
    OutputPreset destination = shaped();
    const std::string before = preset_to_json(destination);
    std::string error;

    CHECK(!preset_from_json("pas du json", destination, error));
    CHECK(!error.empty());
    CHECK(!preset_from_json(R"({"version":99})", destination, error));
    CHECK(!preset_from_json(
        R"({"version":1,"mesh":{"cols":3,"rows":3,"points":[]}})", destination, error));

    CHECK_EQ(preset_to_json(destination), before);  // untouched by every failure
}

SVJ_TEST("preset: a grid larger than the editor allows is refused") {
    OutputPreset out;
    std::string error;
    const std::string oversized =
        R"({"version":1,"mesh":{"cols":99,"rows":2,"points":[]}})";
    CHECK(!preset_from_json(oversized, out, error));
    CHECK(!error.empty());
}
