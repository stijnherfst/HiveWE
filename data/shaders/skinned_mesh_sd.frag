#version 450 core

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require
#include "skinned_mesh_common.glsl"
#include "skinned_mesh_fragment.glsl"

layout (location = 0) in vec2 UV;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec4 vertexColor;
layout (location = 3) in vec3 team_color;
layout (location = 4) flat in int layer_index;

layout (location = 0, index = 0) out vec4 color;
layout (location = 0, index = 1) out vec4 blend_factor;

void main() {
	SkinnedFrame frame = pc.frame;
	const LayerTextureIds ids = frame.layer_textures.values[layer_index];
	const LayerParams p = frame.layer_params.values[layer_index];
	cull_back_face(p);
	const vec4 diffuse = texture(textures[nonuniformEXT(ids.albedo)], UV);

	if (p.is_team_color != 0u) {
		color = vec4(team_color * diffuse.r, 1.f) * vertexColor;
	} else {
		color = diffuse * vertexColor;
	}

	if (vertexColor.a == 0.0 || color.a < p.alpha_test) {
		discard;
	}

	if (p.layer_lit != 0u && frame.render_lighting != 0u) {
		const float contribution = (dot(Normal, -frame.light_direction.xyz) + 1.f) * 0.5f;
		color.rgb *= clamp(contribution, 0.f, 1.f);
	}

	blend(p, color, blend_factor);
}
