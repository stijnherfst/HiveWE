#version 450 core

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

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

// Tightly packed vertex streams
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec2Buffer {
	vec2 values[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec3Buffer {
	vec3 values[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec4Buffer {
	vec4 values[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Uvec4Buffer {
	uvec4 values[];
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
	Vec3Buffer positions;
	Vec2Buffer uvs;
	Vec3Buffer normals;
	Vec4Buffer tangents;
	/// 4 bone indices then 4 weights, 16 bits each
	Uvec4Buffer skins;
} pc;

layout (location = 0) out vec2 UV;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec4 vertexColor;
layout (location = 3) out vec3 team_color;

const vec3 team_colors[28] = {
	vec3(1.000, 0.012, 0.012),
	vec3(0.000, 0.259, 1.000),
	vec3(0.106, 0.906, 0.729),
	vec3(0.333, 0.000, 0.506),
	vec3(0.996, 0.988, 0.000),
	vec3(0.996, 0.537, 0.051),
	vec3(0.129, 0.749, 0.000),
	vec3(0.894, 0.361, 0.686),
	vec3(0.576, 0.584, 0.588),
	vec3(0.494, 0.749, 0.945),
	vec3(0.063, 0.384, 0.278),
	vec3(0.310, 0.169, 0.020),
	vec3(0.612, 0.000, 0.000),
	vec3(0.000, 0.000, 0.765),
	vec3(0.000, 0.922, 1.000),
	vec3(0.741, 0.000, 1.000),
	vec3(0.925, 0.808, 0.529),
	vec3(0.969, 0.647, 0.545),
	vec3(0.749, 1.000, 0.506),
	vec3(0.859, 0.722, 0.922),
	vec3(0.310, 0.314, 0.333),
	vec3(0.925, 0.941, 1.000),
	vec3(0.000, 0.471, 0.118),
	vec3(0.647, 0.435, 0.204),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
};

void main() {
	MeshFrameData frame = pc.frame;
	const vec3 vPosition = pc.positions.values[gl_VertexIndex];
	const vec2 vUV = pc.uvs.values[gl_VertexIndex];
	const vec3 vNormal = pc.normals.values[gl_VertexIndex];
	const uvec4 vSkin = pc.skins.values[gl_VertexIndex];
	const mat4 b0 = frame.bones[int(vSkin.x & 0x0000FFFFu)];
	const mat4 b1 = frame.bones[int(vSkin.x >> 16)];
	const mat4 b2 = frame.bones[int(vSkin.y & 0x0000FFFFu)];
	const mat4 b3 = frame.bones[int(vSkin.y >> 16)];
	const float w0 = (vSkin.z & 0x0000FFFFu) / 255.f;
	const float w1 = (vSkin.z >> 16) / 255.f;
	const float w2 = (vSkin.w & 0x0000FFFFu) / 255.f;
	const float w3 = (vSkin.w >> 16) / 255.f;
	const mat4 skin_matrix = b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3;

	gl_Position = frame.mvp * skin_matrix * vec4(vPosition, 1.f);

	UV = vUV;
	Normal = vNormal;
	vertexColor = pc.layer_color;
	team_color = team_colors[frame.team_color_index];
}
