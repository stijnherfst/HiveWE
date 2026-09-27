#version 450 core

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// Mirrors ImDrawVert
struct ImDrawVert {
	vec2 position;
	vec2 uv;
	/// RGBA8, red in the lowest byte
	uint color;
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer ImDrawVertBuffer {
	ImDrawVert values[];
};

// Mirrors ImGuiPushConstants in vulkan_imgui.cpp
layout(push_constant, std430) uniform PushConstants {
	vec2 scale;
	vec2 translate;
	uint texture_slot;
	ImDrawVertBuffer vertices;
} pc;

layout (location = 0) out vec2 uv;
layout (location = 1) out vec4 color;

void main() {
	const ImDrawVert vertex = pc.vertices.values[gl_VertexIndex];
	uv = vertex.uv;
	color = unpackUnorm4x8(vertex.color);
	gl_Position = vec4(vertex.position * pc.scale + pc.translate, 0.0, 1.0);
}
