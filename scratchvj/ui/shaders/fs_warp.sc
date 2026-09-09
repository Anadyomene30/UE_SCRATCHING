$input v_texcoord0, v_color0

// scratchvj — the program through the mask, onto the output screen.
//
// The mask's coverage arrives in the vertex colour's alpha and is multiplied
// into the picture: outside the mask the projector shows black, which on a
// wall is the same as showing nothing. The feathered edge is interpolated
// across the 32x32 grid rather than evaluated per pixel -- a soft edge a few
// pixels wide on a 4K output is beyond what a grid that size resolves, so a
// feather is drawn slightly coarser than the preview promises. Said here
// because nobody should hunt for that difference.

#include <bgfx_shader.sh>

SAMPLER2D(s_source, 0);

void main()
{
	vec3 colour = texture2D(s_source, v_texcoord0).rgb;
	gl_FragColor = vec4(colour * v_color0.a, 1.0);
}
