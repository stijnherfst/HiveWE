#version 450 core

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

// Written once per model per frame. Mirrors MeshFrameData in vk_editable_mesh.ixx.
layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer MeshFrameData {
	mat4 mvp;
	vec4 light_direction;
	uint team_color_index;
	uint _pad0;
	uint _pad1;
	uint _pad2;
	mat4 bones[];
};

// Mirrors MeshPushConstants in vk_editable_mesh.ixx
layout(push_constant, std430) uniform PushConstants {
	MeshFrameData frame;
	// Bindless slots. SD uses only the first; HD uses albedo, normal, ORM, emissive, team color.
	uint texture_slots[5];
	// Bit 0: lighting, bit 1: the layer is a team color/glow texture
	uint flags;
	vec4 layer_color;
	float alpha_test;
} pc;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout (location = 0) in vec2 UV;
layout (location = 1) in vec3 tangent_light_direction;
layout (location = 2) in vec4 vertexColor;
layout (location = 3) in vec3 team_color;

layout (location = 0) out vec4 color;

void main() {
	color = texture(textures[pc.texture_slots[0]], UV) * vertexColor;

	if (vertexColor.a == 0.0 || color.a < pc.alpha_test) {
		discard;
	}

	if ((pc.flags & 1u) != 0u) {
		vec3 emissive_texel = texture(textures[pc.texture_slots[3]], UV).rgb;
		vec4 orm_texel = texture(textures[pc.texture_slots[2]], UV);
		vec3 tc_texel = texture(textures[pc.texture_slots[4]], UV).rgb;
		color.rgb = (color.rgb * (1 - orm_texel.w) + color.rgb * tc_texel.r * team_color * orm_texel.w);

		vec2 normal_texel = texture(textures[pc.texture_slots[1]], UV).xy * 2.0 - 1.0;
		vec3 N = vec3(normal_texel, sqrt(max(0.0, 1.0 - dot(normal_texel, normal_texel))));

		float lambert = clamp(dot(N, -tangent_light_direction), 0.f, 1.f);
		color.rgb *= clamp(lambert + 0.1, 0.f, 1.f);
		color.rgb += emissive_texel;
	}
}
