#include "core/sphere.h"

#include <cmath>

namespace svj {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

double radians(double degrees) { return degrees * kPi / 180.0; }

double wrap01(double v) {
    const double f = v - std::floor(v);
    return f < 0.0 ? f + 1.0 : f;
}

Vec3 rotate_z(const Vec3& v, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return Vec3{v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

Vec3 rotate_x(const Vec3& v, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return Vec3{v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3 rotate_y(const Vec3& v, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return Vec3{v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

}  // namespace

double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 normalise(const Vec3& v) {
    const double length = std::sqrt(dot(v, v));
    if (length <= 1e-12) return Vec3{0.0, 0.0, -1.0};
    return Vec3{v.x / length, v.y / length, v.z / length};
}

Vec2 equirect_from_direction(const Vec3& direction) {
    const Vec3 d = normalise(direction);
    const double longitude = std::atan2(d.x, -d.z);   // 0 straight ahead, +x to the right
    const double latitude = std::asin(std::fmin(1.0, std::fmax(-1.0, d.y)));
    return Vec2{wrap01(longitude / kTwoPi + 0.5), 0.5 - latitude / kPi};
}

Vec3 direction_from_equirect(const Vec2& uv) {
    const double longitude = (wrap01(uv.u) - 0.5) * kTwoPi;
    const double latitude = (0.5 - uv.v) * kPi;
    const double cos_lat = std::cos(latitude);
    return Vec3{cos_lat * std::sin(longitude), std::sin(latitude), -cos_lat * std::cos(longitude)};
}

Vec3 apply_view_rotation(const SphereView& view, const Vec3& direction) {
    // Roll first, about the view axis; then pitch, then yaw. Applied in that order
    // the three read the way a camera head moves rather than fighting each other.
    Vec3 d = rotate_z(direction, radians(view.roll_deg));
    d = rotate_x(d, radians(view.pitch_deg));
    // Negated so that POSITIVE YAW TURNS RIGHT. A knob turned clockwise has to
    // pan the view to the right; the mathematically natural sign about +Y does
    // the opposite, and an inverted pan knob is a usability defect, not a taste.
    d = rotate_y(d, -radians(view.yaw_deg));
    return d;
}

Vec3 view_direction(const SphereView& view, const Vec2& uv) {
    Vec3 camera;

    switch (view.projection) {
        case Projection::Perspective: {
            // A planar projection cannot reach 180 degrees, so the field of view
            // is clamped short of it rather than blowing up to infinity.
            const double fov = std::fmin(std::fmax(view.fov_deg, 1.0), 170.0);
            const double half = std::tan(radians(fov) * 0.5);
            const double x = (uv.u - 0.5) * 2.0 * half;
            const double y = (0.5 - uv.v) * 2.0 * half / std::fmax(view.aspect, 1e-6);
            camera = normalise(Vec3{x, y, -1.0});
            break;
        }
        case Projection::Fisheye: {
            // Equidistant: distance from the centre is proportional to the angle
            // away from the axis, so a full hemisphere and more fits in the disc.
            const double zoom = std::fmax(view.planet_zoom, 1e-6);
            const double x = (uv.u - 0.5) * 2.0 / zoom;
            const double y = (0.5 - uv.v) * 2.0 / (zoom * std::fmax(view.aspect, 1e-6));
            const double r = std::sqrt(x * x + y * y);
            const double theta = r * kPi;  // r = 1 reaches straight behind
            const double sin_t = std::sin(theta);
            const double denom = r > 1e-12 ? r : 1.0;
            camera = normalise(Vec3{x / denom * sin_t, y / denom * sin_t, -std::cos(theta)});
            break;
        }
        case Projection::LittlePlanet: {
            // Stereographic, looking straight down: the centre of the image is the
            // ground underfoot and the horizon curls into a circle around it.
            const double zoom = std::fmax(view.planet_zoom, 1e-6);
            const double x = (uv.u - 0.5) * 2.0 / zoom;
            const double y = (0.5 - uv.v) * 2.0 / (zoom * std::fmax(view.aspect, 1e-6));
            const double r = std::sqrt(x * x + y * y);
            const double theta = 2.0 * std::atan(r);
            const double sin_t = std::sin(theta);
            const double denom = r > 1e-12 ? r : 1.0;
            // Down the -Y axis, with the screen plane spread around it.
            camera = normalise(Vec3{x / denom * sin_t, -std::cos(theta), -y / denom * sin_t});
            break;
        }
    }

    return apply_view_rotation(view, camera);
}

Vec2 sample_equirect(const SphereView& view, const Vec2& uv) {
    return equirect_from_direction(view_direction(view, uv));
}

}  // namespace svj
