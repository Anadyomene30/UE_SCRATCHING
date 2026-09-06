$input v_texcoord0

// scratchvj — one eye of the sphere, as a headset asks for it.
//
// A transcription of core/headset.cpp, the way fs_view360.sc transcribes
// core/sphere.cpp, and tools/eye_check holds it to that reference pixel for
// pixel. It is a separate shader from fs_view360 rather than another branch in
// it because a headset never wants little planet or fisheye, and never wants a
// symmetric field of view -- sharing the code would mean the flat view paying
// per fragment for a case it does not have.
//
// Two differences from the flat pass, both forced by real headsets:
//
// - The field of view is FOUR tangents, not one half-angle. Every headset lens
//   sees further towards the nose than away from it, so the centre pixel is not
//   the view axis. Interpolating between the bounds is what puts it right.
// - The head is a QUATERNION, applied before the performer's rotation of the
//   sphere: the headset is the head, the performer is the world.

#include <bgfx_shader.sh>

SAMPLER2D(s_equirect, 0);

// The eye's field of view as tangents: x left, y right, z up, w down.
uniform vec4 u_eye_fov;
// The eye's orientation in play space, as a quaternion (x, y, z, w).
uniform vec4 u_eye_rot;
// The performer's rotation of the sphere: x yaw (radians), y pitch, z roll.
uniform vec4 u_gaze;

vec3 rotate_quat(vec4 q, vec3 v)
{
	// v + 2w(u x v) + 2(u x (u x v)) -- the same form core/headset uses, and
	// cheaper than building a matrix for a single vector.
	vec3 u = q.xyz;
	vec3 uv = cross(u, v);
	return v + 2.0 * (q.w * uv + cross(u, uv));
}

vec3 rotate_zxy(vec3 v, float roll, float pitch, float yaw)
{
	float c = cos(roll);  float s = sin(roll);
	v = vec3(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
	c = cos(pitch); s = sin(pitch);
	v = vec3(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
	c = cos(-yaw); s = sin(-yaw);
	return vec3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}

void main()
{
	float u = v_texcoord0.x;
	float v = v_texcoord0.y;

	// The plane one unit ahead, spanned by the four reported bounds.
	float x = u_eye_fov.x + (u_eye_fov.y - u_eye_fov.x) * u;
	float y = u_eye_fov.z + (u_eye_fov.w - u_eye_fov.z) * v;
	vec3 ray = normalize(vec3(x, y, -1.0));

	vec3 d = rotate_quat(u_eye_rot, ray);
	d = rotate_zxy(d, u_gaze.z, u_gaze.y, u_gaze.x);

	float longitude = atan2(d.x, -d.z);
	float latitude = asin(clamp(d.y, -1.0, 1.0));
	float eu = fract(longitude / 6.28318530717959 + 0.5);
	float ev = 0.5 - latitude / 3.14159265358979;

	gl_FragColor = texture2D(s_equirect, vec2(eu, ev));
}
