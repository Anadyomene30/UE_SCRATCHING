#include "core/headset.h"

#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;

}  // namespace

FovTangents fov_from_angles(double left_rad, double right_rad, double up_rad,
                            double down_rad) {
    // Clamped short of a right angle on each side. A planar projection cannot
    // reach 90 degrees off-axis -- the tangent runs to infinity -- and a runtime
    // that reports a degenerate field of view during startup would otherwise
    // produce infinities in the per-pixel path rather than a wrong-but-finite
    // picture. `core/sphere` clamps its own field of view for the same reason.
    const double limit = kPi * 0.5 - 1e-3;
    const auto clamp = [limit](double a) { return std::fmin(std::fmax(a, -limit), limit); };
    return FovTangents{std::tan(clamp(left_rad)), std::tan(clamp(right_rad)),
                       std::tan(clamp(up_rad)), std::tan(clamp(down_rad))};
}

FovTangents fov_from_horizontal(double fov_deg, double aspect) {
    const double fov = std::fmin(std::fmax(fov_deg, 1.0), 170.0);
    const double half = std::tan(fov * kPi / 180.0 * 0.5);
    const double vertical = half / std::fmax(aspect, 1e-6);
    return FovTangents{-half, half, vertical, -vertical};
}

Quat normalise(const Quat& q) {
    const double length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (length <= 1e-12) return Quat{};
    return Quat{q.x / length, q.y / length, q.z / length, q.w / length};
}

Vec3 rotate(const Quat& q, const Vec3& v) {
    const Quat n = normalise(q);
    // v + 2w(u x v) + 2(u x (u x v)), with u the vector part. Cheaper than
    // building a matrix for one vector, and it cannot drift the way repeated
    // matrix products do.
    const Vec3 u{n.x, n.y, n.z};
    const Vec3 uv{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
    const Vec3 uuv{u.y * uv.z - u.z * uv.y, u.z * uv.x - u.x * uv.z,
                   u.x * uv.y - u.y * uv.x};
    return Vec3{v.x + 2.0 * (n.w * uv.x + uuv.x), v.y + 2.0 * (n.w * uv.y + uuv.y),
                v.z + 2.0 * (n.w * uv.z + uuv.z)};
}

Quat concat(const Quat& b, const Quat& a) {
    return Quat{b.w * a.x + b.x * a.w + b.y * a.z - b.z * a.y,
                b.w * a.y - b.x * a.z + b.y * a.w + b.z * a.x,
                b.w * a.z + b.x * a.y - b.y * a.x + b.z * a.w,
                b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z};
}

Quat quat_from_yaw_deg(double yaw_deg) {
    // Negated about +Y for the same reason `core/sphere` negates its yaw: a knob
    // turned clockwise has to pan the view RIGHT, and the mathematically natural
    // sign about +Y pans it left. That inversion was a real defect once; keeping
    // the two files' conventions identical is what stops it coming back.
    const double half = -yaw_deg * kPi / 180.0 * 0.5;
    return Quat{0.0, std::sin(half), 0.0, std::cos(half)};
}

Vec3 eye_direction(const SphereView& world, const EyeView& eye, const Vec2& uv) {
    // The pixel as a point on the plane one unit ahead. With an asymmetric field
    // of view the CENTRE PIXEL IS NOT THE VIEW AXIS -- it is the middle of the
    // reported bounds, which sits off-axis on every real headset. Interpolating
    // between the bounds rather than scaling a half-angle is the whole point of
    // taking four numbers instead of one.
    const double x = eye.fov.left + (eye.fov.right - eye.fov.left) * uv.u;
    const double y = eye.fov.up + (eye.fov.down - eye.fov.up) * uv.v;

    const Vec3 ray = normalise(Vec3{x, y, -1.0});
    const Vec3 in_play_space = rotate(eye.orientation, ray);

    // Then the performer's rotation of the sphere. Reusing sphere's own function
    // rather than restating it means the knob behaves identically whether the
    // picture is going to a screen or to a headset.
    return apply_view_rotation(world, in_play_space);
}

Vec2 sample_equirect_eye(const SphereView& world, const EyeView& eye, const Vec2& uv) {
    return equirect_from_direction(eye_direction(world, eye, uv));
}

}  // namespace svj
