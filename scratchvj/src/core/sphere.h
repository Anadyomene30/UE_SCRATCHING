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

// The three reprojections a viewer computes from the sphere. The names are the
// canonical ones of spec/00-vocabulaire.md, "Les modes de vue, ou
// reprojections" (SCRATCHVJ-02): `rectilinear` rather than `perspective`, and
// `fisheye_view` rather than `fisheye`, which already names a FILE projection
// with its own parameters. A view is not a file format.
enum class Projection {
    Rectilinear,    // the default view of a viewer
    LittlePlanet,   // stereographic, looking down: the whole sphere in one disc
    FisheyeView,    // equidistant, a very wide circular view
};

// `viewer.geometry.pitch_deg_clamp` of design/tokens.json: the pitch is bounded
// at +/-89.9 and not at 90, whatever drives it. At the exact pole the yaw
// becomes undefined and the view jumps; a knob pushed to its end lands on the
// pole exactly as a mouse would (SCRATCHVJ-16). `viewer.geometry` carries THIS
// AND NOTHING ELSE -- it is what is required of every viewer, whatever it looks
// at; the three field values live under `viewer.plate` (SCRATCHVJ-25 Q1, which
// corrected two sources that wrote the wrong path).
constexpr double kPitchClampDeg = 89.9;

struct SphereView {
    double yaw_deg = 0.0;       // positive turns the view to the right
    double pitch_deg = 0.0;     // bounded by kPitchClampDeg wherever it is set
    double roll_deg = 0.0;
    // `lines.scene.viewer.plate.fov_deg_default` of design/tokens.json, and the
    // stage cadran is why it is not the 75 of the workshop: 75 is written for a
    // monitor at 60 cm where a plate is JUDGED, 90 is what is PROJECTED to an
    // audience. Reading distance is variable 1 of the cadran, so the resting
    // field is re-set by the room and not by a repo (SCRATCHVJ-25 Q2). The
    // BOUNDS are a different question and do NOT come down with it: 20-170 is
    // justified by little planet, which is a reason of REPROJECTION and not of
    // room, and they wait for the `Domaine` column of the reprojection table
    // (SCRATCHVJ-02) -- see engine.cpp and the gaze popup in ui/panels.cpp.
    double fov_deg = 90.0;      // horizontal, for Rectilinear
    double aspect = 16.0 / 9.0;
    Projection projection = Projection::Rectilinear;
    double planet_zoom = 1.0;   // LittlePlanet and FisheyeView framing
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
