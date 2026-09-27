#version 450 core

layout(push_constant, std430) uniform PushConstants {
	mat4 mvp;
	vec4 color;
} pc;

layout (location = 0) out vec4 color;

void main() {
	color = pc.color;
}
