#include <cmath>

#include "core/warp.h"
#include "harness.h"

using namespace svj;

namespace {

void check_point(const Point& got, const Point& want, double tolerance = 1e-9) {
    CHECK_NEAR(got.x, want.x, tolerance);
    CHECK_NEAR(got.y, want.y, tolerance);
}

CornerPin keystone() {
    // A projector aimed from below and off to one side: the top edge is narrower
    // than the bottom, which is the everyday case this exists for.
    CornerPin pin;
    pin.top_left = {0.20, 0.05};
    pin.top_right = {0.85, 0.12};
    pin.bottom_right = {0.95, 0.90};
    pin.bottom_left = {0.05, 0.80};
    return pin;
}

}  // namespace

SVJ_TEST("warp: an unpinned quad is the identity") {
    CornerPin pin;
    CHECK(pin.is_identity());
    const Homography h = homography_from(pin);
    check_point(apply(h, Point{0.37, 0.62}), Point{0.37, 0.62});
}

SVJ_TEST("warp: the four corners land exactly on the four corners") {
    const CornerPin pin = keystone();
    const Homography h = homography_from(pin);
    check_point(apply(h, Point{0.0, 0.0}), pin.top_left);
    check_point(apply(h, Point{1.0, 0.0}), pin.top_right);
    check_point(apply(h, Point{1.0, 1.0}), pin.bottom_right);
    check_point(apply(h, Point{0.0, 1.0}), pin.bottom_left);
}

SVJ_TEST("warp: THE CENTRE DOES NOT LAND ON THE CENTROID") {
    // The point of doing this as a homography. Perspective compresses the far
    // side, so the middle of the picture sits nearer the narrow edge. A bilinear
    // stretch would put it at the centroid and shear everything inside.
    const CornerPin pin = keystone();
    const Homography h = homography_from(pin);
    const Point centre = apply(h, Point{0.5, 0.5});

    const Point centroid{
        (pin.top_left.x + pin.top_right.x + pin.bottom_right.x + pin.bottom_left.x) / 4.0,
        (pin.top_left.y + pin.top_right.y + pin.bottom_right.y + pin.bottom_left.y) / 4.0};

    CHECK(std::hypot(centre.x - centroid.x, centre.y - centroid.y) > 1e-3);
}

SVJ_TEST("warp: straight lines stay straight") {
    // The defining property of a projective transform, and what a bilinear warp
    // gets wrong.
    const Homography h = homography_from(keystone());
    const Point a = apply(h, Point{0.0, 0.3});
    const Point b = apply(h, Point{0.5, 0.3});
    const Point c = apply(h, Point{1.0, 0.3});

    // b must sit on the line through a and c.
    const double cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    CHECK_NEAR(cross, 0.0, 1e-12);
}

SVJ_TEST("warp: a parallelogram stays affine, with no projective term") {
    CornerPin pin;
    pin.top_left = {0.1, 0.1};
    pin.top_right = {0.9, 0.2};
    pin.bottom_right = {1.0, 0.9};
    pin.bottom_left = {0.2, 0.8};
    const Homography h = homography_from(pin);
    CHECK_NEAR(h.m[6], 0.0, 1e-12);
    CHECK_NEAR(h.m[7], 0.0, 1e-12);
    // And then the centre IS the centroid, because there is no perspective.
    check_point(apply(h, Point{0.5, 0.5}), Point{0.55, 0.5}, 1e-9);
}

SVJ_TEST("warp: the inverse takes points back where they came from") {
    const Homography h = homography_from(keystone());
    bool ok = false;
    const Homography back = invert(h, ok);
    CHECK(ok);

    for (double u = 0.0; u <= 1.0; u += 0.25) {
        for (double v = 0.0; v <= 1.0; v += 0.25) {
            const Point round_trip = apply(back, apply(h, Point{u, v}));
            check_point(round_trip, Point{u, v}, 1e-9);
        }
    }
}

SVJ_TEST("warp: a collinear quad is refused rather than producing infinities") {
    // Dragging three corners onto a line has no projection. Returning the
    // identity keeps a picture on the screen; a matrix of infinities would paint
    // nothing at all, mid-set, with no way to tell why.
    CornerPin flat;
    flat.top_left = {0.0, 0.5};
    flat.top_right = {0.5, 0.5};
    flat.bottom_right = {1.0, 0.5};
    flat.bottom_left = {0.0, 0.5};

    const Homography h = homography_from(flat);
    const Point p = apply(h, Point{0.5, 0.5});
    CHECK(std::isfinite(p.x));
    CHECK(std::isfinite(p.y));
}

SVJ_TEST("warp: a singular matrix reports that it cannot be inverted") {
    Homography singular;
    singular.m[0] = 0.0;
    singular.m[4] = 0.0;
    singular.m[8] = 0.0;
    bool ok = true;
    invert(singular, ok);
    CHECK(!ok);
}

SVJ_TEST("mask: no polygon means nothing is masked") {
    Mask mask;
    CHECK(mask.empty());
    CHECK_NEAR(mask.coverage(Point{0.5, 0.5}), 1.0, 1e-9);
    CHECK_NEAR(mask.coverage(Point{-10.0, 40.0}), 1.0, 1e-9);
}

SVJ_TEST("mask: a hard-edged polygon is in or out") {
    Mask mask;
    mask.set_polygon({{0.2, 0.2}, {0.8, 0.2}, {0.8, 0.8}, {0.2, 0.8}});
    CHECK_NEAR(mask.coverage(Point{0.5, 0.5}), 1.0, 1e-9);
    CHECK_NEAR(mask.coverage(Point{0.05, 0.5}), 0.0, 1e-9);
    CHECK_NEAR(mask.coverage(Point{0.5, 0.95}), 0.0, 1e-9);
}

SVJ_TEST("mask: a feathered edge ramps across the outline") {
    Mask mask;
    mask.set_polygon({{0.2, 0.2}, {0.8, 0.2}, {0.8, 0.8}, {0.2, 0.8}});
    mask.set_feather(0.1);

    CHECK_NEAR(mask.coverage(Point{0.5, 0.5}), 1.0, 1e-9);   // well inside
    CHECK_NEAR(mask.coverage(Point{0.2, 0.5}), 0.5, 1e-9);   // exactly on the line
    CHECK_NEAR(mask.coverage(Point{0.0, 0.5}), 0.0, 1e-9);   // well outside

    const double just_inside = mask.coverage(Point{0.22, 0.5});
    CHECK(just_inside > 0.5);
    CHECK(just_inside < 1.0);
}

SVJ_TEST("mask: a concave outline is handled, not just a convex one") {
    // An L shape: the notch has to read as outside.
    Mask mask;
    mask.set_polygon({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.4},
                      {0.4, 0.4}, {0.4, 1.0}, {0.0, 1.0}});
    CHECK_NEAR(mask.coverage(Point{0.2, 0.2}), 1.0, 1e-9);
    CHECK_NEAR(mask.coverage(Point{0.8, 0.2}), 1.0, 1e-9);
    CHECK_NEAR(mask.coverage(Point{0.8, 0.8}), 0.0, 1e-9);  // the notch
}

SVJ_TEST("mask: fewer than three points is not a polygon") {
    Mask mask;
    mask.set_polygon({{0.0, 0.0}, {1.0, 1.0}});
    CHECK(mask.empty());
    CHECK_NEAR(mask.coverage(Point{0.5, 0.5}), 1.0, 1e-9);
}

SVJ_TEST("warp: pin and mask compose the way the output pipeline uses them") {
    // A pixel is warped, then tested against the mask, which is the order the
    // final shader runs them in.
    const Homography h = homography_from(keystone());
    Mask mask;
    mask.set_polygon({{0.1, 0.1}, {0.9, 0.1}, {0.9, 0.9}, {0.1, 0.9}});

    const Point centre = apply(h, Point{0.5, 0.5});
    CHECK_NEAR(mask.coverage(centre), 1.0, 1e-9);

    const Point corner = apply(h, Point{0.0, 0.0});
    CHECK_NEAR(mask.coverage(corner), 0.0, 1e-9);  // pinned outside the mask
}
