#version 450 core

layout (location = 0) in vec3 vPosition;

layout (location = 1) in float vOffset;

layout (binding = 0) uniform sampler2D color_texture;

layout (location = 3) uniform mat4 MVP;



out vec3 Color;

void main() { 

	vec4(vPosition)

	gl_Position = MVP * vec4(vPosition, 1);
	Color = vColor;
}