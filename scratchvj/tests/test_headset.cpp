#include <cmath>

#include "core/headset.h"
#include "core/sphere.h"
#include "harness.h"

using namespace svj;

namespace {

constexpr double kPi = 3.14159265358979323846;

double radians(double degrees) { return degrees * kPi / 180.0; }

// A roll about the view axis, which is how a canted display differs from a
// level one.
Quat quat_from_roll_deg(double roll_deg) {
    const double half = radians(roll_deg) * 0.5;
    return Quat{0.0, 0.0, std::sin(half), std::cos(half)};
}

Quat quat_from_pitch_deg(double pitch_deg) {
    const double half = radians(pitch_deg) * 0.5;
    return Quat{std::sin(half), 0.0, 0.0, std::cos(half)};
}

void check_direction(const Vec3& got, const Vec3& want, double tolerance = 1e-9) {
    CHECK_NEAR(got.x, want.x, tolerance);
    CHECK_NEAR(got.y, want.y, tolerance);
    CHECK_NEAR(got.z, want.z, tolerance);
}

}  // namespace

// The load-bearing test. `sphere_check` holds the shader to core/sphere, so
// anchoring the headset path to core/sphere means the headset inherits that
// proof instead of needing a second one. If this ever fails, the flat view and
// the VR view have started disagreeing about where the performer is looking.
SVJ_TEST("headset: a still head reproduces the flat perspective view exactly") {
    SphereView world;
    world.yaw_deg = 37.0;
    world.pitch_deg = -12.0;
    world.roll_deg = 5.0;
    world.fov_deg = 90.0;
    world.aspect = 16.0 / 9.0;

    EyeView eye;
    eye.orientation = Quat{};  // identity: the head has not moved
    eye.fov = fov_from_horizontal(world.fov_deg, world.aspect);

    for (double u = 0.05; u < 1.0; u += 0.15) {
        for (double v = 0.05; v < 1.0; v += 0.15) {
            const Vec2 uv{u, v};
            const Vec2 flat = sample_equirect(world, uv);
            const Vec2 vr = sample_equirect_eye(world, eye, uv);
            CHECK_NEAR(vr.u, flat.u, 1e-12);
            CHECK_NEAR(vr.v, flat.v, 1e-12);
        }
    }
}

SVJ_TEST("headset: turning the head right turns the gaze right") {
    SphereView world;  // performer neutral, so only the head moves
    EyeView eye;
    eye.fov = fov_from_horizontal(90.0, 1.0);

    eye.orientation = Quat{};
    const Vec3 ahead = eye_direction(world, eye, Vec2{0.5, 0.5});
    check_direction(ahead, Vec3{0.0, 0.0, -1.0});

    eye.orientation = quat_from_yaw_deg(90.0);
    const Vec3 right = eye_direction(world, eye, Vec2{0.5, 0.5});
    check_direction(right, Vec3{1.0, 0.0, 0.0});
}

SVJ_TEST("headset: looking up reaches the top of the equirect frame") {
    SphereView world;
    EyeView eye;
    eye.fov = fov_from_horizontal(90.0, 1.0);
    eye.orientation = quat_from_pitch_deg(90.0);

    const Vec2 uv = sample_equirect_eye(world, eye, Vec2{0.5, 0.5});
    CHECK_NEAR(uv.v, 0.0, 1e-9);  // v = 0 is the north pole
}

// The decision this module exists to encode: the headset is the head and the
// performer is the world, so the two rotations compose rather than compete.
SVJ_TEST("headset: the performer's rotation and the head's rotation compose") {
    EyeView eye;
    eye.fov = fov_from_horizontal(80.0, 1.6);

    SphereView spun;
    spun.yaw_deg = 50.0;
    eye.orientation = quat_from_yaw_deg(30.0);
    const Vec3 both = eye_direction(spun, eye, Vec2{0.3, 0.7});

    // The same result with the performer neutral and the whole rotation in the
    // head: spinning the sphere 50 degrees under a head turned 30 is a head
    // turned 80 in a still world.
    SphereView still;
    EyeView combined;
    combined.fov = eye.fov;
    combined.orientation = concat(quat_from_yaw_deg(50.0), quat_from_yaw_deg(30.0));
    const Vec3 folded = eye_direction(still, combined, Vec2{0.3, 0.7});

    check_direction(both, folded, 1e-12);
}

// The reason four numbers replace one. Every real headset reports a field of
// view that is wider towards the nose than away from it, and a projection that
// scales a single half-angle silently mis-aims by that difference.
SVJ_TEST("headset: an asymmetric field of view does not aim through the centre pixel") {
    SphereView world;
    EyeView eye;
    eye.orientation = Quat{};
    eye.fov = fov_from_angles(radians(-50.0), radians(40.0), radians(45.0),
                              radians(-45.0));

    // The centre PIXEL is the middle of the reported bounds, which is off-axis.
    const Vec3 centre = eye_direction(world, eye, Vec2{0.5, 0.5});
    CHECK(std::fabs(centre.x) > 0.05);

    // The view AXIS is wherever the horizontal tangent crosses zero, which is
    // not the middle of the image.
    const double axis_u = -eye.fov.left / (eye.fov.right - eye.fov.left);
    CHECK(std::fabs(axis_u - 0.5) > 0.01);
    const Vec3 on_axis = eye_direction(world, eye, Vec2{axis_u, 0.5});
    CHECK_NEAR(on_axis.x, 0.0, 1e-12);
}

// Quest 3 and Quest Pro cant their two panels. Sharing one pose between the eyes
// tilts one image, and a tilted horizon in VR is felt long before it is seen.
SVJ_TEST("headset: each eye carries its own orientation, so canted panels stay level") {
    SphereView world;
    const FovTangents fov = fov_from_horizontal(90.0, 1.0);

    EyeView left;
    left.fov = fov;
    left.orientation = quat_from_roll_deg(6.0);
    EyeView right;
    right.fov = fov;
    right.orientation = quat_from_roll_deg(-6.0);

    const Vec2 uv{0.8, 0.2};
    const Vec2 sample_left = sample_equirect_eye(world, left, uv);
    const Vec2 sample_right = sample_equirect_eye(world, right, uv);
    CHECK(std::fabs(sample_left.u - sample_right.u) > 1e-4);
    CHECK(std::fabs(sample_left.v - sample_right.v) > 1e-4);
}

// A runtime reporting a degenerate field of view -- which happens during session
// startup, before the first frame is synchronised -- must not put infinities in
// the per-pixel path.
SVJ_TEST("headset: a right-angle field of view stays finite") {
    const FovTangents fov =
        fov_from_angles(-kPi * 0.5, kPi * 0.5, kPi * 0.5, -kPi * 0.5);
    CHECK(std::isfinite(fov.left));
    CHECK(std::isfinite(fov.right));
    CHECK(std::isfinite(fov.up));
    CHECK(std::isfinite(fov.down));

    SphereView world;
    EyeView eye;
    eye.fov = fov;
    const Vec3 d = eye_direction(world, eye, Vec2{0.0, 1.0});
    CHECK(std::isfinite(d.x));
    CHECK(std::isfinite(d.y));
    CHECK(std::isfinite(d.z));
}

SVJ_TEST("headset: rotating by a quaternion keeps a direction unit length") {
    const Quat q = normalise(Quat{0.3, -0.5, 0.2, 0.8});
    const Vec3 v = normalise(Vec3{0.4, 0.7, -0.6});
    const Vec3 turned = rotate(q, v);
    CHECK_NEAR(std::sqrt(dot(turned, turned)), 1.0, 1e-12);
}

SVJ_TEST("headset: an unnormalised pose is still usable") {
    // Runtimes are allowed to hand back a pose that has drifted slightly off
    // unit length. Normalising inside rotate() means that costs accuracy rather
    // than scaling the whole world.
    const Vec3 v{0.0, 0.0, -1.0};
    const Quat exact = quat_from_yaw_deg(45.0);
    const Quat scaled{exact.x * 1.01, exact.y * 1.01, exact.z * 1.01, exact.w * 1.01};
    check_direction(rotate(scaled, v), rotate(exact, v), 1e-12);
}
