#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in uvec2 vSkin;

layout (location = 0) uniform mat4 VP;
layout (location = 3) uniform mat4 M;

void main() {
	gl_Position = VP * M * vec4(vPosition, 1);
}