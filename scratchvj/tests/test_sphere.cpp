#include <cmath>

#include "core/sphere.h"
#include "harness.h"

using namespace svj;

namespace {

const Vec2 kCentre{0.5, 0.5};

void check_direction(const Vec3& got, const Vec3& want, double tolerance = 1e-9) {
    CHECK_NEAR(got.x, want.x, tolerance);
    CHECK_NEAR(got.y, want.y, tolerance);
    CHECK_NEAR(got.z, want.z, tolerance);
}

}  // namespace

SVJ_TEST("sphere: equirect and direction are inverses of each other") {
    for (double u = 0.02; u < 1.0; u += 0.07) {
        for (double v = 0.02; v < 1.0; v += 0.07) {
            const Vec2 uv{u, v};
            const Vec2 back = equirect_from_direction(direction_from_equirect(uv));
            CHECK_NEAR(back.u, u, 1e-9);
            CHECK_NEAR(back.v, v, 1e-9);
        }
    }
}

SVJ_TEST("sphere: the centre of the equirect frame looks straight ahead") {
    check_direction(direction_from_equirect(kCentre), Vec3{0.0, 0.0, -1.0});
}

SVJ_TEST("sphere: the poles are the top and bottom edges") {
    check_direction(direction_from_equirect(Vec2{0.5, 0.0}), Vec3{0.0, 1.0, 0.0});
    check_direction(direction_from_equirect(Vec2{0.5, 1.0}), Vec3{0.0, -1.0, 0.0});
}

SVJ_TEST("sphere: longitude runs the full turn across the frame") {
    check_direction(direction_from_equirect(Vec2{0.75, 0.5}), Vec3{1.0, 0.0, 0.0});
    check_direction(direction_from_equirect(Vec2{0.25, 0.5}), Vec3{-1.0, 0.0, 0.0});
    // The seam wraps rather than clamping.
    const Vec3 left = direction_from_equirect(Vec2{0.0, 0.5});
    const Vec3 right = direction_from_equirect(Vec2{1.0, 0.5});
    check_direction(left, right, 1e-9);
}

SVJ_TEST("sphere: the centre pixel of a perspective view is the view direction") {
    SphereView view;
    view.yaw_deg = 37.0;
    view.pitch_deg = -12.0;
    const Vec2 uv = sample_equirect(view, kCentre);

    // Yaw of 37 degrees is 37/360 of a turn to the right of centre.
    CHECK_NEAR(uv.u, 0.5 + 37.0 / 360.0, 1e-9);
    CHECK_NEAR(uv.v, 0.5 + 12.0 / 180.0, 1e-9);
}

SVJ_TEST("sphere: yaw wraps around the seam instead of running off the frame") {
    SphereView view;
    view.yaw_deg = 200.0;
    const Vec2 uv = sample_equirect(view, kCentre);
    CHECK(uv.u >= 0.0);
    CHECK(uv.u <= 1.0);
    CHECK_NEAR(uv.u, 0.5 + 200.0 / 360.0 - 1.0, 1e-9);

    view.yaw_deg = 560.0;  // 200 plus a full turn: the same view
    const Vec2 again = sample_equirect(view, kCentre);
    CHECK_NEAR(again.u, uv.u, 1e-9);
}

SVJ_TEST("sphere: a wider field of view covers more of the sphere") {
    SphereView narrow;
    narrow.fov_deg = 40.0;
    SphereView wide = narrow;
    wide.fov_deg = 120.0;

    const double narrow_span = sample_equirect(narrow, Vec2{1.0, 0.5}).u -
                               sample_equirect(narrow, Vec2{0.0, 0.5}).u;
    const double wide_span = sample_equirect(wide, Vec2{1.0, 0.5}).u -
                             sample_equirect(wide, Vec2{0.0, 0.5}).u;
    CHECK(wide_span > narrow_span);
    CHECK(narrow_span > 0.0);
}

SVJ_TEST("sphere: an absurd field of view is clamped rather than blowing up") {
    // A planar projection cannot reach 180 degrees; the tangent would run away.
    SphereView view;
    view.fov_deg = 179.9;
    const Vec3 d = view_direction(view, Vec2{0.0, 0.5});
    CHECK(std::isfinite(d.x));
    CHECK(std::isfinite(d.z));
    CHECK_NEAR(std::sqrt(dot(d, d)), 1.0, 1e-9);
}

SVJ_TEST("sphere: roll turns the picture without moving where it points") {
    SphereView flat;
    SphereView rolled;
    rolled.roll_deg = 90.0;

    // The centre is unmoved by roll.
    const Vec2 a = sample_equirect(flat, kCentre);
    const Vec2 b = sample_equirect(rolled, kCentre);
    CHECK_NEAR(a.u, b.u, 1e-9);
    CHECK_NEAR(a.v, b.v, 1e-9);

    // But a point above the centre moves to the side.
    const Vec2 top_flat = sample_equirect(flat, Vec2{0.5, 0.2});
    const Vec2 top_rolled = sample_equirect(rolled, Vec2{0.5, 0.2});
    CHECK(std::fabs(top_flat.v - 0.5) > 0.01);
    CHECK_NEAR(top_rolled.v, 0.5, 1e-9);
}

SVJ_TEST("sphere: pitching all the way up lands on the pole") {
    SphereView view;
    view.pitch_deg = 90.0;
    const Vec2 uv = sample_equirect(view, kCentre);
    CHECK_NEAR(uv.v, 0.0, 1e-9);
}

SVJ_TEST("sphere: every pixel of every projection yields a unit direction") {
    for (const Projection projection :
         {Projection::Perspective, Projection::LittlePlanet, Projection::Fisheye}) {
        SphereView view;
        view.projection = projection;
        view.yaw_deg = 23.0;
        view.pitch_deg = 11.0;
        view.roll_deg = 7.0;
        for (double u = 0.0; u <= 1.0; u += 0.1) {
            for (double v = 0.0; v <= 1.0; v += 0.1) {
                const Vec3 d = view_direction(view, Vec2{u, v});
                CHECK_NEAR(std::sqrt(dot(d, d)), 1.0, 1e-9);
                const Vec2 uv = equirect_from_direction(d);
                CHECK(uv.u >= 0.0 && uv.u <= 1.0);
                CHECK(uv.v >= -1e-12 && uv.v <= 1.0 + 1e-12);
            }
        }
    }
}

SVJ_TEST("sphere: a little planet looks straight down at its centre") {
    SphereView view;
    view.projection = Projection::LittlePlanet;
    const Vec3 d = view_direction(view, kCentre);
    check_direction(d, Vec3{0.0, -1.0, 0.0}, 1e-9);
    CHECK_NEAR(sample_equirect(view, kCentre).v, 1.0, 1e-9);
}

SVJ_TEST("sphere: a little planet reaches the horizon and beyond") {
    SphereView view;
    view.projection = Projection::LittlePlanet;
    view.aspect = 1.0;

    // Moving out from the centre tips the view up towards the horizon, steadily.
    double previous = -1.001;  // the centre already sits at exactly -1
    for (double u = 0.5; u <= 0.951; u += 0.05) {
        const Vec3 d = view_direction(view, Vec2{u, 0.5});
        CHECK(d.y > previous);  // rising out of the ground, never doubling back
        previous = d.y;
    }

    // "And beyond" is about the CORNERS: they sit further out than the edges, and
    // that is where a little planet shows sky above the horizon.
    const Vec3 corner = view_direction(view, Vec2{1.0, 1.0});
    CHECK(corner.y > 0.0);
    const Vec3 edge = view_direction(view, Vec2{1.0, 0.5});
    CHECK(corner.y > edge.y);
}

SVJ_TEST("sphere: a fisheye centre looks straight ahead and its edge looks behind") {
    SphereView view;
    view.projection = Projection::Fisheye;
    view.aspect = 1.0;
    check_direction(view_direction(view, kCentre), Vec3{0.0, 0.0, -1.0}, 1e-9);

    const Vec3 edge = view_direction(view, Vec2{1.0, 0.5});
    CHECK(edge.z > 0.9);  // r = 1 is straight behind
}

SVJ_TEST("sphere: zooming a little planet changes how much sphere fits") {
    SphereView tight;
    tight.projection = Projection::LittlePlanet;
    tight.aspect = 1.0;
    tight.planet_zoom = 2.0;

    SphereView loose = tight;
    loose.planet_zoom = 0.5;

    const Vec3 a = view_direction(tight, Vec2{0.9, 0.5});
    const Vec3 b = view_direction(loose, Vec2{0.9, 0.5});
    CHECK(b.y > a.y);  // the looser framing has already climbed further up
}

SVJ_TEST("sphere: a degenerate direction does not divide by zero") {
    const Vec3 d = normalise(Vec3{0.0, 0.0, 0.0});
    CHECK_NEAR(std::sqrt(dot(d, d)), 1.0, 1e-9);
}

SVJ_TEST("sphere: the view direction is independent of anything to do with time") {
    // The point of the feature: the hand scratches time while a knob moves the
    // gaze. Two identical views must agree whatever else is going on.
    SphereView a;
    a.yaw_deg = 34.0;
    a.pitch_deg = -8.0;
    a.fov_deg = 90.0;
    SphereView b = a;
    const Vec2 first = sample_equirect(a, Vec2{0.3, 0.7});
    const Vec2 second = sample_equirect(b, Vec2{0.3, 0.7});
    CHECK_NEAR(first.u, second.u, 1e-12);
    CHECK_NEAR(first.v, second.v, 1e-12);
}

SVJ_TEST("sphere: positive yaw turns RIGHT and positive pitch turns UP") {
    // Fixed handedness, pinned. A knob that pans the wrong way is a usability
    // defect rather than a matter of taste, and it is invisible in a still.
    SphereView view;
    view.yaw_deg = 30.0;
    CHECK(sample_equirect(view, Vec2{0.5, 0.5}).u > 0.5);

    view.yaw_deg = -30.0;
    CHECK(sample_equirect(view, Vec2{0.5, 0.5}).u < 0.5);

    view.yaw_deg = 0.0;
    view.pitch_deg = 30.0;
    CHECK(sample_equirect(view, Vec2{0.5, 0.5}).v < 0.5);  // v counts down from the top

    view.pitch_deg = -30.0;
    CHECK(sample_equirect(view, Vec2{0.5, 0.5}).v > 0.5);
}

SVJ_TEST("sphere: looking right shows what was to the right in the frame") {
    // The end-to-end statement of the same thing, in terms of the image.
    SphereView ahead;
    SphereView right;
    right.yaw_deg = 45.0;
    CHECK(sample_equirect(right, Vec2{0.5, 0.5}).u >
          sample_equirect(ahead, Vec2{0.5, 0.5}).u);
}
