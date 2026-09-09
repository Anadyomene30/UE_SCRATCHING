$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

// scratchvj — the program onto a surface that is not square to the projector.
//
// The geometry is done on the CPU: a 32x32 grid over the source, each vertex
// placed by the corner pin's homography or by the warp mesh (core/warp,
// core/mesh -- the very functions the SORTIE screen previews with), and its
// colour's alpha set to the mask's coverage there. This shader only passes
// them through, so what the output does and what the preview drew cannot
// disagree: they are the same numbers.

#include <bgfx_shader.sh>

void main()
{
	gl_Position = vec4(a_position.xy, 0.0, 1.0);
	v_texcoord0 = a_texcoord0;
	v_color0 = a_color0;
}
