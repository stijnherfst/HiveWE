#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable


layout (binding = 0) uniform sampler2DArray textureArray;

layout (location = 4) uniform int current_texture;

in vec2 UV;
in vec4 Color;

out vec4 outColor;

void main() {
	outColor = texture(textureArray, vec3(UV, current_texture)) * Color;
}