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
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec4 vertexColor;
layout (location = 3) in vec3 team_color;

layout (location = 0) out vec4 color;

void main() {
	const vec4 texel = texture(textures[pc.texture_slots[0]], UV);
	if ((pc.flags & 2u) != 0u) {
		color = vec4(team_color * texel.r, 1.f) * vertexColor;
	} else {
		color = texel * vertexColor;
	}

	if (vertexColor.a == 0.0 || color.a < pc.alpha_test) {
		discard;
	}

	if ((pc.flags & 1u) != 0u) {
		float contribution = (dot(Normal, -pc.frame.light_direction.xyz) + 1.f) * 0.5f;
		color.rgb *= clamp(contribution, 0.f, 1.f);
	}
}
