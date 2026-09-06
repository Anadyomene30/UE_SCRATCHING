$input v_texcoord0

// scratchvj — the program compositor, on the GPU.
//
// This is core/compose.cpp transcribed, not reinterpreted: deck A lands
// Normal over black, deck B rides additively (with the constant-power
// crossfader that is a fade, and a transform cut never passes through black),
// and the overlay applies its own blend mode above them, outside the
// crossfader. The CPU version is the tested reference; tools/gpu_check holds
// this shader to it, channel for channel. Change one, and the check fails
// until you change the other -- that is the point.
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

void main()
{
	vec4 a = texture2D(s_deckA, v_texcoord0);
	vec4 b = texture2D(s_deckB, v_texcoord0);
	vec4 o = texture2D(s_overlay, v_texcoord0);

	// A over black, Normal: mix(0, a, gain) is just a scale.
	vec3 c = a.rgb * u_gains.x;

	// B, Add: blended = min(under + over, 1), then eased in by its gain.
	vec3 addB = min(c + b.rgb, vec3_splat(1.0));
	c = mix(c, addB, u_gains.y);

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
