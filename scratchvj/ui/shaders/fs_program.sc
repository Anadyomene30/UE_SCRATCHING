$input v_texcoord0

// scratchvj — the program compositor, on the GPU.
//
// This is core/compose.cpp transcribed, not reinterpreted: the two decks
// through the crossfader's transition (compose_decks), then the overlay by
// its own blend mode above them, outside the crossfader. The CPU version is
// the tested reference; tools/gpu_check holds this shader to it, channel for
// channel, transition by transition. Change one, and the check fails until
// you change the other -- that is the point.
//
// The deck samplers read the BC1 cache textures directly; the hardware decodes
// blocks in the sampler, which is why the analysis pass emits BCn at all.

#include <bgfx_shader.sh>

SAMPLER2D(s_deckA, 0);
SAMPLER2D(s_deckB, 1);
SAMPLER2D(s_overlay, 2);

// x: gain A, y: gain B, z: gain overlay, w: overlay blend mode
// (0 Normal, 1 Add, 2 Multiply, 3 Screen, 4 Alpha -- core/mixer.h's BlendMode)
uniform vec4 u_gains;
// x: transition (core/mixer.h's Transition, 0 Cut .. 8 Zoom), y: crossfader
// position 0..1 after reverse. z, w unused.
uniform vec4 u_xfade;

float luma(vec3 rgb) { return 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b; }

void main()
{
	vec2 uv = v_texcoord0;
	vec4 a = texture2D(s_deckA, uv);
	vec4 b = texture2D(s_deckB, uv);
	vec4 o = texture2D(s_overlay, uv);

	float ga = u_gains.x;
	float gb = u_gains.y;
	float p = clamp(u_xfade.y, 0.0, 1.0);
	float t = u_xfade.x;
	vec3 wa = a.rgb * ga;
	vec3 wb = b.rgb * gb;

	// Additive by default: B added to A, eased in by its weight.
	vec3 c = mix(wa, min(wa + b.rgb, vec3_splat(1.0)), gb);
	if (t < 0.5)
	{
		// Cut
		c = p < 0.5 ? wa : wb;
	}
	else if (t < 1.5)
	{
		// Fade: B, Normal over A, by its weight.
		c = mix(wa, b.rgb, gb);
	}
	else if (t > 2.5 && t < 4.5)
	{
		// Multiply (3), Screen (4): A, the blend, B.
		vec3 blended = t < 3.5 ? wa * wb : vec3_splat(1.0) - (vec3_splat(1.0) - wa) * (vec3_splat(1.0) - wb);
		c = p < 0.5 ? mix(wa, blended, p * 2.0) : mix(blended, wb, p * 2.0 - 1.0);
	}
	else if (t > 4.5 && t < 5.5)
	{
		// Luma wipe: A's dark pixels give way first.
		float th = p * 1.16 - 0.08;
		float k = smoothstep(th - 0.08, th + 0.08, luma(wa));
		c = mix(wb, wa, k);
	}
	else if (t > 5.5 && t < 6.5)
	{
		// Geometric wipe, left to right, soft edge.
		float th = p * 1.06 - 0.03;
		float k = smoothstep(th - 0.03, th + 0.03, uv.x);
		c = mix(wb, wa, k);
	}
	else if (t > 6.5 && t < 7.5)
	{
		// RGB split: the channels cross one after the other, red first.
		vec3 k = clamp(vec3_splat(p * 3.0) - vec3(0.0, 1.0, 2.0), vec3_splat(0.0), vec3_splat(1.0));
		c = mix(wa, wb, k);
	}
	else if (t > 7.5)
	{
		// Zoom: A zooms in as it leaves, B fades up underneath.
		float zoom = 1.0 + p * 1.5;
		vec2 zuv = vec2_splat(0.5) + (uv - vec2_splat(0.5)) / zoom;
		vec3 az = texture2D(s_deckA, zuv).rgb * ga;
		c = mix(az, wb, p);
	}

	// Overlay, by mode. Branches on a uniform are free enough here: one draw,
	// every fragment takes the same path.
	float mode = u_gains.w;
	vec3 blended = o.rgb;  // Normal, and the colour part of Alpha
	if (mode > 0.5 && mode < 1.5)
	{
		blended = min(c + o.rgb, vec3_splat(1.0));  // Add
	}
	else if (mode > 1.5 && mode < 2.5)
	{
		blended = c * o.rgb;  // Multiply
	}
	else if (mode > 2.5 && mode < 3.5)
	{
		blended = vec3_splat(1.0) - (vec3_splat(1.0) - c) * (vec3_splat(1.0) - o.rgb);  // Screen
	}
	float weight = u_gains.z;
	if (mode > 3.5)
	{
		weight = weight * o.a;  // Alpha reads the source's own alpha
	}
	c = mix(c, blended, weight);

	gl_FragColor = vec4(c, 1.0);
}
