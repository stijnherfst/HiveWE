#version 450 core

// Mirrors SelectionPushConstants in src/render/vk_brush_overlay.ixx
layout(push_constant, std430) uniform PushConstants {
	mat4 MVP;
	/// 0: rectangle outline as a 5 vertex line strip, 1: filled square as 2 triangles
	uint shape;
} pc;

layout (location = 0) out vec2 uv;

const vec2 outline[5] = vec2[5](vec2(1, 1), vec2(0, 1), vec2(0, 0), vec2(1, 0), vec2(1, 1));
const vec2 square[6] = vec2[6](vec2(1, 1), vec2(0, 1), vec2(0, 0), vec2(0, 0), vec2(1, 0), vec2(1, 1));

void main() {
	const vec2 position = pc.shape == 0u ? outline[gl_VertexIndex] : square[gl_VertexIndex];
	uv = position;
	gl_Position = pc.MVP * vec4(position, 0, 1);
}
