#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in mat4 vInstance;

out vec2 UV;

void main() {
	gl_Position = vInstance * vec4(vPosition, 1);
	UV = vUV;
}