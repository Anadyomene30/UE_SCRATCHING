#include <cmath>

#include "core/mesh.h"
#include "harness.h"

using namespace svj;

namespace {

double distance(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// The worst the surface moves over a dense sweep of the source square.
double worst_shift(const WarpMesh& before, const WarpMesh& after) {
    double worst = 0.0;
    for (int i = 0; i <= 40; ++i) {
        for (int j = 0; j <= 40; ++j) {
            const double u = i / 40.0;
            const double v = j / 40.0;
            worst = std::max(worst, distance(before.map(u, v), after.map(u, v)));
        }
    }
    return worst;
}

}  // namespace

SVJ_TEST("mesh: a fresh grid is the identity, whatever its size") {
    for (const int n : {2, 3, 5, 9}) {
        WarpMesh mesh;
        mesh.reset(n, n);
        CHECK(mesh.is_identity());
        CHECK_NEAR(mesh.map(0.25, 0.75).x, 0.25, 1e-9);
        CHECK_NEAR(mesh.map(0.25, 0.75).y, 0.75, 1e-9);
    }
}

SVJ_TEST("mesh: THE SURFACE PASSES THROUGH ITS CONTROL POINTS, IN BOTH MODES") {
    // A point you drag must land where you dropped it. If the surface only came
    // near its handles, the tool would argue with the hand holding it -- which
    // is the reason this uses Catmull-Rom rather than a B-spline that merely
    // approaches its points.
    WarpMesh mesh;
    mesh.reset(4, 3);
    mesh.set(1, 1, Point{0.45, 0.62});
    mesh.set(2, 0, Point{0.70, -0.08});

    for (const WarpInterpolation mode :
         {WarpInterpolation::Bilinear, WarpInterpolation::Bezier}) {
        mesh.set_interpolation(mode);
        for (int row = 0; row < mesh.rows(); ++row) {
            for (int col = 0; col < mesh.cols(); ++col) {
                const double u = static_cast<double>(col) / (mesh.cols() - 1);
                const double v = static_cast<double>(row) / (mesh.rows() - 1);
                CHECK(distance(mesh.map(u, v), mesh.at(col, row)) < 1e-9);
            }
        }
    }
}

SVJ_TEST("mesh: ADDING A POINT DOES NOT MOVE A STRAIGHT-EDGED PICTURE") {
    // The property that decides whether this is usable mid-show: reaching for
    // more control must not lurch what is already aimed at a wall. In Bilinear
    // the new points are sampled from the surface they subdivide, so the shape
    // is preserved to the last bit.
    WarpMesh mesh;
    mesh.reset(3, 3);
    mesh.set_interpolation(WarpInterpolation::Bilinear);
    mesh.set(1, 1, Point{0.62, 0.38});
    mesh.set(0, 0, Point{0.05, 0.02});
    mesh.set(2, 2, Point{0.98, 0.93});

    WarpMesh before = mesh;
    CHECK(mesh.insert_column(1));
    CHECK_EQ(mesh.cols(), 4);
    CHECK(worst_shift(before, mesh) < 1e-9);

    before = mesh;
    CHECK(mesh.insert_row(1));
    CHECK_EQ(mesh.rows(), 4);
    CHECK(worst_shift(before, mesh) < 1e-9);
}

SVJ_TEST("mesh: adding a point to a curved picture moves it under half a percent") {
    // Bezier cannot promise the exact preservation Bilinear does, and the
    // reason is a trade this design makes on purpose. A scheme can interpolate
    // its control points exactly, or it can be exactly preserved under knot
    // insertion; not both, not simply. A hand dragging a point cares far more
    // about the first -- a handle that does not land where it was dropped is
    // unusable -- so insertion is the one that gives, and it gives 0.39% of the
    // output's width. The bound below is the measured figure with headroom, not
    // an aspiration: the non-uniform Barry-Goldman form brought it down from
    // 1.5%, and if a change pushes it back up this test says so.
    WarpMesh mesh;
    mesh.reset(4, 4);
    mesh.set_interpolation(WarpInterpolation::Bezier);
    mesh.set(1, 1, Point{0.40, 0.28});
    mesh.set(2, 2, Point{0.72, 0.75});

    const WarpMesh before = mesh;
    CHECK(mesh.insert_column(2));
    const double shift = worst_shift(before, mesh);
    CHECK(shift < 0.005);
}

SVJ_TEST("mesh: bezier bends where bilinear creases") {
    // The reason both modes exist. Across a cell boundary the bilinear surface
    // changes direction abruptly; the Catmull-Rom one does not. Measured as the
    // turn in the tangent either side of the seam.
    WarpMesh mesh;
    mesh.reset(3, 3);
    mesh.set(1, 1, Point{0.5, 0.30});  // pull the middle up
    mesh.set(1, 0, Point{0.5, -0.15});

    const double v = 0.5;
    const double eps = 1e-4;
    const auto turn = [&](WarpInterpolation mode) {
        mesh.set_interpolation(mode);
        // Tangents just before and just after the seam at u = 0.5.
        const Point a0 = mesh.map(0.5 - 2 * eps, v);
        const Point a1 = mesh.map(0.5 - eps, v);
        const Point b0 = mesh.map(0.5 + eps, v);
        const Point b1 = mesh.map(0.5 + 2 * eps, v);
        const double before_slope = (a1.y - a0.y) / (a1.x - a0.x);
        const double after_slope = (b1.y - b0.y) / (b1.x - b0.x);
        return std::fabs(after_slope - before_slope);
    };

    CHECK(turn(WarpInterpolation::Bilinear) > 0.5);   // a crease
    CHECK(turn(WarpInterpolation::Bezier) < 0.05);    // smooth through it
}

SVJ_TEST("mesh: the outer edges of the grid cannot be removed") {
    // Removing a border column would move the picture's edge, which is never
    // what "remove a point" is asked to mean.
    WarpMesh mesh;
    mesh.reset(4, 4);
    CHECK(!mesh.remove_column(0));
    CHECK(!mesh.remove_column(3));
    CHECK(!mesh.remove_row(0));
    CHECK(mesh.remove_column(1));
    CHECK_EQ(mesh.cols(), 3);
}

SVJ_TEST("mesh: a two-by-two grid is never reduced further") {
    WarpMesh mesh;
    mesh.reset(2, 2);
    CHECK(!mesh.remove_column(1));
    CHECK(!mesh.remove_row(1));
    CHECK_EQ(mesh.cols(), 2);
    CHECK_EQ(mesh.rows(), 2);
}

SVJ_TEST("mesh: the grid refuses to grow past the size a finger can aim at") {
    WarpMesh mesh;
    mesh.reset(kMaxWarpDivisions, 2);
    CHECK(!mesh.insert_column(1));
    CHECK_EQ(mesh.cols(), kMaxWarpDivisions);
}

SVJ_TEST("mesh: a moved point stops the grid calling itself the identity") {
    WarpMesh mesh;
    mesh.reset(3, 3);
    CHECK(mesh.is_identity());
    mesh.set(1, 1, Point{0.51, 0.5});
    CHECK(!mesh.is_identity());
}
