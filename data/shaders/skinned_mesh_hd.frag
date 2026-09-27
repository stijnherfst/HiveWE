#version 450 core

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require
#include "skinned_mesh_common.glsl"
#include "skinned_mesh_fragment.glsl"

layout (location = 0) in vec2 UV;
layout (location = 1) in vec3 tangent_light_direction;
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

	color = texture(textures[nonuniformEXT(ids.albedo)], UV) * vertexColor;

	if (color.a < p.alpha_test) {
		discard;
	}

	if (p.layer_lit != 0u && frame.render_lighting != 0u) {
		const vec3 emissive_texel = texture(textures[nonuniformEXT(ids.emissive)], UV).rgb;
		const vec4 orm_texel = texture(textures[nonuniformEXT(ids.orm)], UV);
		const vec3 tc_texel = texture(textures[nonuniformEXT(ids.team_color)], UV).rgb;
		color.rgb = (color.rgb * (1 - orm_texel.w) + color.rgb * tc_texel.r * team_color * orm_texel.w);

		// normal is a 2 channel normal map so we have to deduce the 3rd value
		const vec2 normal_texel = texture(textures[nonuniformEXT(ids.normal)], UV).xy * 2.0 - 1.0;
		const vec3 normal = vec3(normal_texel, sqrt(1.0 - dot(normal_texel, normal_texel)));

		const float lambert = clamp(dot(normal, -tangent_light_direction), 0.f, 1.f);
		color.rgb *= clamp(lambert + 0.1, 0.f, 1.f);
		color.rgb += emissive_texel;
	}

	blend(p, color, blend_factor);
}
