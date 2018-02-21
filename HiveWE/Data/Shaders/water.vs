#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec4 vColor;

layout (location = 3) uniform mat4 MVP;

out vec2 UV;
out vec4 Color;

void main() { 
	gl_Position = MVP * vec4(vPosition, 1);
	UV = vUV;
	Color = vColor;
}