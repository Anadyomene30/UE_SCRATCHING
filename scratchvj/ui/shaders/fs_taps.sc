$input v_texcoord0

// scratchvj — trails and slit scan, from the clip at eight moments at once.
//
// The moments are chosen by core/videotaps, which is where the design argument
// lives: a tap is where the picture WAS, computed from position and velocity,
// never remembered from a previous frame. This shader only reads them.
//
//   Trails    sum the layers with the weights the plan carries. That is a
//             tapped delay line, the same equation as the audio delay the unit
//             is paired with, so one knob really does move both.
//   Slit scan bands the picture: each vertical band shows a different moment.
//             Eight bands rather than one column per moment, and calling it
//             bands rather than a true scan is the honest description -- a
//             per-column scan needs one frame per column, which is a thousand
//             uploads a frame, not eight.

#include <bgfx_shader.sh>

SAMPLER2DARRAY(s_taps, 0);

// x: mode (0 trails, 1 slit scan), y: mix, z: tap count, w: unused.
uniform vec4 u_taps;
// The eight weights, packed two vec4s deep.
uniform vec4 u_weights[2];

float weight_of(int k)
{
	// Indexing a uniform array by a runtime value is not portable across every
	// backend bgfx targets, so the eight are unrolled into a comparison chain.
	if (k == 0) { return u_weights[0].x; }
	if (k == 1) { return u_weights[0].y; }
	if (k == 2) { return u_weights[0].z; }
	if (k == 3) { return u_weights[0].w; }
	if (k == 4) { return u_weights[1].x; }
	if (k == 5) { return u_weights[1].y; }
	if (k == 6) { return u_weights[1].z; }
	return u_weights[1].w;
}

void main()
{
	vec2 uv = v_texcoord0;
	vec3 dry = texture2DArray(s_taps, vec3(uv, 0.0)).rgb;
	vec3 wet = dry;
	float count = u_taps.z;

	if (u_taps.x < 0.5)
	{
		wet = vec3_splat(0.0);
		for (int k = 0; k < 8; ++k)
		{
			if (float(k) >= count) { break; }
			wet += texture2DArray(s_taps, vec3(uv, float(k))).rgb * weight_of(k);
		}
	}
	else
	{
		// Which band this column falls in, and therefore which moment it shows.
		float band = floor(uv.x * count);
		band = min(band, count - 1.0);
		wet = texture2DArray(s_taps, vec3(uv, band)).rgb;
	}

	gl_FragColor = vec4(mix(dry, wet, u_taps.y), 1.0);
}
