$input v_texcoord0

// scratchvj — the program onto the output screen, and nothing else.
//
// No mix, no effect, no colour work: everything that shapes the picture has
// already happened by the time it gets here, and the projector must show
// exactly what the interface previews and what Spout publishes. Letterboxing
// is done by the view rect on the CPU rather than here, so a screen whose
// shape differs from the program's gets black bars rather than a stretch.

#include <bgfx_shader.sh>

SAMPLER2D(s_source, 0);

void main()
{
	gl_FragColor = vec4(texture2D(s_source, v_texcoord0).rgb, 1.0);
}
