#version 450 core

#extension GL_EXT_buffer_reference : require

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Mat4Buffer {
	mat4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Uvec2Buffer {
	uvec2 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Uvec4Buffer {
	uvec4 values[];
};

// Mirrors PickPushConstants in src/base/render_manager.ixx
layout(push_constant, std430) uniform PushConstants {
	mat4 MVP;
	Mat4Buffer bones;
	Uvec2Buffer vertices;
	Uvec4Buffer skins;
	int color_id;
} pc;

/// Needs to match the extent that it was packed with
vec3 unpack_uvec2_to_vec3(const uvec2 v, const float extent) {
	const uint x = v.y >> 11;
	const uint y = ((v.y & 0x7FFu) << 10) + (v.x >> 22);
	const uint z = v.x & 0x3FFFFFu;

	const float xf = (float(x) / float(1u << 21) * 2.f - 1.f) * extent;
	const float yf = (float(y) / float(1u << 21) * 2.f - 1.f) * extent;
	const float zf = (float(z) / float(1u << 22) * 2.f - 1.f) * extent;

	return vec3(xf, yf, zf);
}

void main() {
	const uvec4 skin = pc.skins.values[gl_VertexIndex];
	const mat4 b0 = pc.bones.values[int(skin.x & 0x0000FFFFu)];
	const mat4 b1 = pc.bones.values[int(skin.x >> 16)];
	const mat4 b2 = pc.bones.values[int(skin.y & 0x0000FFFFu)];
	const mat4 b3 = pc.bones.values[int(skin.y >> 16)];
	const float w0 = (skin.z & 0x0000FFFFu) / 255.f;
	const float w1 = (skin.z >> 16) / 255.f;
	const float w2 = (skin.w & 0x0000FFFFu) / 255.f;
	const float w3 = (skin.w >> 16) / 255.f;

	vec4 position = vec4(unpack_uvec2_to_vec3(pc.vertices.values[gl_VertexIndex], 8192.f), 1.f);
	position = b0 * position * w0 + b1 * position * w1 + b2 * position * w2 + b3 * position * w3;
	position.w = 1.f;

	gl_Position = pc.MVP * position;
}
