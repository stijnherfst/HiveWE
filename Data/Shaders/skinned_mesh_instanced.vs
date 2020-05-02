#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

layout (location = 4) uniform mat4 MVP;
layout (location = 5) uniform mat4 bones[10];

out vec2 UV;

void main() {
	gl_Position = MVP * vec4(vPosition / 64.f + vec3(32, 32, 0), 1);
	UV = vUV;
}