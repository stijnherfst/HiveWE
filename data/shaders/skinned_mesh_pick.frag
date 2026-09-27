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

layout (location = 0) out vec4 color;

void main() {
	const int id = pc.color_id;
	color = vec4((id & 0xFF) / 255.f, ((id & 0xFF00) >> 8) / 255.f, ((id & 0xFF0000) >> 16) / 255.f, 1.f);
}
