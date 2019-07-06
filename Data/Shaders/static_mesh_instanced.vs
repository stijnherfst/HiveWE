#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in mat4 vInstance;

layout (location = 4) uniform mat4 VP;

out vec2 UV;

void main() {
	gl_Position = VP * vInstance * vec4(vPosition, 1);
	UV = vUV;
}