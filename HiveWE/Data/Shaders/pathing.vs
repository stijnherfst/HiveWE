#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;

layout (location = 2) uniform mat4 MVP;

out vec3 Color;

void main() { 

	vec4(vPosition)

	gl_Position = MVP * vec4(vPosition, 1);
	Color = vColor;
}