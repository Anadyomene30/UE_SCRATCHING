// scratchvj — turning a 360 frame into the flat view you actually watch.
//
// The cache stores equirectangular frames exactly as they were shot; only the
// RENDERING changes. For each pixel a ray direction is built from yaw, pitch, roll
// and field of view, and that direction picks a point on the equirect image. In a
// shader this is a handful of instructions, which is why 360 costs almost nothing
// here beyond video memory.
//
// The consequence worth having is that the view direction is INDEPENDENT of the
// playback position: the hand scratches time while a knob moves the gaze. Nothing
// in this file knows anything about time, and that is the point.
//
// Conventions: right-handed, +Y up, -Z forward at yaw and pitch zero. Equirect u
// runs 0..1 over longitude -pi..pi, and v runs 0 at the top (+pi/2 latitude) to 1
// at the bottom.
#pragma once

namespace svj {

struct Vec2 {
    double u = 0.0;
    double v = 0.0;
};

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

enum class Projection {
    Perspective,    // the ordinary flat view
    LittlePlanet,   // stereographic, looking down: the whole sphere in one disc
    Fisheye,        // equidistant, a very wide circular view
};

struct SphereView {
    double yaw_deg = 0.0;       // positive turns the view to the right
    double pitch_deg = 0.0;
    double roll_deg = 0.0;
    double fov_deg = 90.0;      // horizontal, for Perspective
    double aspect = 16.0 / 9.0;
    Projection projection = Projection::Perspective;
    double planet_zoom = 1.0;   // LittlePlanet and Fisheye framing
};

Vec3 normalise(const Vec3& v);
double dot(const Vec3& a, const Vec3& b);

// Where a world direction lands on the equirectangular frame.
Vec2 equirect_from_direction(const Vec3& direction);

// The direction an equirect texture coordinate names. Inverse of the above.
Vec3 direction_from_equirect(const Vec2& uv);

// The direction seen through a screen pixel, `uv` running 0..1 across the output.
Vec3 view_direction(const SphereView& view, const Vec2& uv);

// The whole reprojection: screen pixel to a point on the equirect frame.
Vec2 sample_equirect(const SphereView& view, const Vec2& uv);

// Rotates a direction by the view's yaw, pitch and roll. Exposed because the same
// rotation applies to anything else placed in the sphere.
Vec3 apply_view_rotation(const SphereView& view, const Vec3& direction);

}  // namespace svj
