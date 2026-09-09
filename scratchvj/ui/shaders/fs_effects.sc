$input v_texcoord0

// scratchvj — one slot of the video effect rack, on the GPU.
//
// A transcription of core/videofx, not a reinterpretation. tools/fx_check runs
// the same pictures through both and compares them channel for channel, so an
// equation that drifts here fails there. The sampling is POINT and the offsets
// are whole texels for exactly that reason: the two can then produce the same
// numbers rather than merely a similar picture.
//
// One slot per pass. Three slots is three passes over a program-sized target,
// which is nothing, and it keeps each effect a self-contained piece rather than
// one growing switch that every new effect makes slower.

#include <bgfx_shader.sh>

SAMPLER2D(s_source, 0);

// x: effect id (see kShaderId in gpu_effects.cpp), y: mix,
// z: the effect's integer parameter (blur step, levels, segments),
// w: unused.
uniform vec4 u_fx;
// xy: one texel in uv, zw: the target's size in pixels.
uniform vec4 u_texel;

// Nine taps a side, always: the radius grows by stepping further between taps
// rather than by taking more of them, so the loop bounds are a constant and the
// cost does not depend on the knob.
#define TAPS 4

vec3 sample_at(vec2 uv)
{
	return texture2D(s_source, clamp(uv, vec2_splat(0.0), vec2_splat(1.0))).rgb;
}

vec3 low_pass(vec2 uv, float step_texels)
{
	float sigma = float(TAPS) * 0.5;
	vec3 sum = vec3_splat(0.0);
	float total = 0.0;
	for (int j = -TAPS; j <= TAPS; ++j)
	{
		for (int i = -TAPS; i <= TAPS; ++i)
		{
			float w = exp(-(float(i * i + j * j)) / (2.0 * sigma * sigma));
			vec2 at = uv + vec2(float(i), float(j)) * step_texels * u_texel.xy;
			sum += sample_at(at) * w;
			total += w;
		}
	}
	return sum / total;
}

void main()
{
	vec2 uv = v_texcoord0;
	vec3 dry = sample_at(uv);
	vec3 wet = dry;

	float id = u_fx.x;
	float p = u_fx.z;

	if (id < 0.5)
	{
		wet = low_pass(uv, p);                       // 0: low pass / blur
	}
	else if (id < 1.5)
	{
		// 1: high pass -- the picture minus its own low pass, lifted to mid
		// grey. The same subtraction the audio side does.
		wet = dry - low_pass(uv, p) + vec3_splat(0.5);
	}
	else if (id < 2.5)
	{
		float n = p - 1.0;                            // 2: bitcrusher / posterise
		wet = floor(dry * n + vec3_splat(0.5)) / n;
	}
	else if (id < 3.5)
	{
		wet = vec3_splat(1.0) - dry;                  // 3: invert
	}
	else if (id < 4.5)
	{
		// 4: mirror. Reading the left half rather than folding in place puts
		// the seam exactly on the centre column whatever the width's parity.
		float x = floor(uv.x * u_texel.z);
		float mirrored = x < u_texel.z * 0.5 ? x : u_texel.z - 1.0 - x;
		wet = sample_at(vec2((mirrored + 0.5) * u_texel.x, uv.y));
	}
	else
	{
		// 5: kaleidoscope. Polar fold, wedges mirrored alternately so the
		// seams meet instead of cutting.
		vec2 c = uv - vec2_splat(0.5);
		float radius = length(c);
		float angle = atan2(c.y, c.x);
		float wedge = 6.28318530717959 / p;
		angle = mod(angle + 6.28318530717959, wedge);
		if (angle > wedge * 0.5) { angle = wedge - angle; }
		vec2 at = vec2(0.5 + cos(angle) * radius, 0.5 + sin(angle) * radius);
		// Snap to the texel core's centre, matching the reference's integer
		// fetch rather than landing wherever the sampler rounds.
		at = (floor(at * u_texel.zw) + vec2_splat(0.5)) * u_texel.xy;
		wet = sample_at(at);
	}

	gl_FragColor = vec4(mix(dry, wet, u_fx.y), 1.0);
}
