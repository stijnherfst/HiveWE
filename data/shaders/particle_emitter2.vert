#version 450 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_color;

layout (location = 0) uniform mat4 MVP;

out vec2 v_uv;
out vec4 v_color;

void main() {
	v_uv = in_uv;
	v_color = in_color;
	gl_Position = MVP * vec4(in_position, 1.0);
}
