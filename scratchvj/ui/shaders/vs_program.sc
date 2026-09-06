$input a_position, a_texcoord0
$output v_texcoord0

// scratchvj — one triangle over the whole target. Three vertices, no quad: a
// quad's diagonal would run the fragment shader twice along it.

#include <bgfx_shader.sh>

void main()
{
	gl_Position = vec4(a_position.xy, 0.0, 1.0);
	v_texcoord0 = a_texcoord0;
}
