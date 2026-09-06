$input v_texcoord0

// scratchvj — the 360 reprojection, on the GPU.
//
// A transcription of core/sphere.cpp, not a reinterpretation: for each output
// pixel, build the ray its projection names (perspective, fisheye equidistant,
// or stereographic little planet), rotate it by roll, pitch, then yaw -- yaw
// negated so a knob turned clockwise pans RIGHT -- and read where that
// direction lands on the equirectangular frame. tools/sphere_check holds this
// shader to the CPU reference, pixel for pixel, through every projection.
//
// The consequence worth having, carried over from the reference: the view
// direction is INDEPENDENT of playback position. The hand scratches time while
// a knob -- or, soon, a headset -- moves the gaze.

#include <bgfx_shader.sh>

SAMPLER2D(s_equirect, 0);

// x: yaw (radians), y: pitch, z: roll, w: projection mode
// (0 perspective, 1 little planet, 2 fisheye -- core/sphere's Projection).
uniform vec4 u_gaze;
// x: tan(fov/2) for perspective, 1/zoom otherwise; y: aspect.
uniform vec4 u_frame;

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
	float mode = u_gaze.w;
	float aspect = max(u_frame.y, 1e-6);

	vec3 cam;
	if (mode < 0.5)
	{
		// Perspective: a plane one unit ahead, spanned by the field of view.
		float half_span = u_frame.x;
		float x = (u - 0.5) * 2.0 * half_span;
		float y = (0.5 - v) * 2.0 * half_span / aspect;
		cam = normalize(vec3(x, y, -1.0));
	}
	else
	{
		float x = (u - 0.5) * 2.0 * u_frame.x;
		float y = (0.5 - v) * 2.0 * u_frame.x / aspect;
		float r = sqrt(x * x + y * y);
		float denom = max(r, 1e-12);
		if (mode < 1.5)
		{
			// Little planet: stereographic, looking straight down. The centre is
			// the ground underfoot, the horizon curls into a circle.
			float theta = 2.0 * atan(r);
			float sin_t = sin(theta);
			cam = normalize(vec3(x / denom * sin_t, -cos(theta), -y / denom * sin_t));
		}
		else
		{
			// Fisheye: equidistant, r proportional to the angle off axis; r = 1
			// reaches straight behind.
			float theta = r * 3.14159265358979;
			float sin_t = sin(theta);
			cam = normalize(vec3(x / denom * sin_t, y / denom * sin_t, -cos(theta)));
		}
	}

	vec3 d = rotate_zxy(cam, u_gaze.z, u_gaze.y, u_gaze.x);

	// Where that direction lands on the equirect frame. atan(x, -z) puts zero
	// longitude straight ahead with +x to the right, exactly as the reference.
	float longitude = atan2(d.x, -d.z);
	float latitude = asin(clamp(d.y, -1.0, 1.0));
	float eu = fract(longitude / 6.28318530717959 + 0.5);
	float ev = 0.5 - latitude / 3.14159265358979;

	gl_FragColor = texture2D(s_equirect, vec2(eu, ev));
}
