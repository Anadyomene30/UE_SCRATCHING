// scratchvj — the geometry of watching the sphere through a headset.
//
// `core/sphere` turns a screen pixel into a direction using yaw, pitch, roll and
// one symmetric field of view. A headset gives neither: it gives a POSE as a
// quaternion and a field of view as four independent half-angles. This file is
// the bridge, and it is separate from `core/sphere` for one reason -- sphere
// knows nothing about who is looking, and adding a headset to it would make the
// flat 360 view pay for a case it does not have.
//
// Who controls what, which is the decision this module exists to encode:
//
//   THE HEADSET IS THE HEAD. THE PERFORMER IS THE WORLD.
//
// In VR the viewer's head already IS the gaze, so the DJ's yaw knob cannot also
// be the gaze without the two fighting over the same degree of freedom. So the
// knob turns the SPHERE instead: the viewer looks freely around a world the
// performer is spinning. A direction therefore composes as
//
//     world = R_gaze( R_eye( ray ) )
//
// -- the eye's ray into play space, then play space into the sphere. R_gaze is
// exactly the rotation `core/sphere` already applies, so the flat view and the
// headset view stay one mechanism rather than two that can disagree.
//
// Two facts about real headsets that shape the interface:
//
// - The eye POSITION is deliberately absent. An equirectangular frame is a
//   sphere at infinity, so no interocular distance can produce parallax from it.
//   Feeding IPD in would not add depth, it would add error. Monoscopic 360 in a
//   headset genuinely looks flat-but-surrounding, and that is the format's
//   property, not a shortcoming here. (Stereoscopic 360 is a different FORMAT --
//   over/under equirect -- and would belong in `core/videocache`, not here.)
//
// - The eye ORIENTATION is per-eye rather than shared. Quest 3 and Quest Pro
//   have canted displays: the two panels do not face the same way, so taking one
//   head pose for both eyes tilts one image. Each eye carries its own.
#pragma once

#include "core/sphere.h"

namespace svj {

// Orientation only, in OpenXR's component order and convention: right-handed,
// +Y up, -Z forward -- the same basis `core/sphere` documents, which is why no
// axis conversion appears anywhere in this file.
struct Quat {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
};

// A field of view as OpenXR reports it: four half-angles measured from the view
// axis, stored as their TANGENTS. Tangents rather than angles because that is
// the form the projection needs, and converting once here keeps a transcendental
// call out of the per-pixel path.
//
// Left and down are normally negative. They are independent: a headset lens sees
// further towards the nose than away from it, so a symmetric field of view is
// the exception in VR rather than the rule.
struct FovTangents {
    double left = -1.0;
    double right = 1.0;
    double up = 1.0;
    double down = -1.0;
};

// Builds tangents from OpenXR's four angles, in radians.
FovTangents fov_from_angles(double left_rad, double right_rad, double up_rad,
                            double down_rad);

// The symmetric field of view `core/sphere` uses, as tangents. Lets one code
// path serve both the flat view and the headset, and lets a test assert the two
// agree instead of hoping they do.
FovTangents fov_from_horizontal(double fov_deg, double aspect);

// One eye: where it points and how much it sees.
struct EyeView {
    Quat orientation;   // eye to play space
    FovTangents fov;
};

Quat normalise(const Quat& q);

// Rotates a direction by `q`.
Vec3 rotate(const Quat& q, const Vec3& v);

// The two rotations composed, applied in that order: `a` then `b`.
Quat concat(const Quat& b, const Quat& a);

// A yaw in degrees as a quaternion, using the same sign convention as
// SphereView::yaw_deg -- positive turns the view to the right.
Quat quat_from_yaw_deg(double yaw_deg);

// The direction seen through the eye's pixel `uv`, in SPHERE space, with
// `world` supplying the performer's rotation of the sphere. Only the rotation
// fields of `world` are read: projection, fov and aspect belong to the flat
// view, and a headset overrides all three.
Vec3 eye_direction(const SphereView& world, const EyeView& eye, const Vec2& uv);

// The whole reprojection for one eye: pixel to a point on the equirect frame.
Vec2 sample_equirect_eye(const SphereView& world, const EyeView& eye, const Vec2& uv);

}  // namespace svj
