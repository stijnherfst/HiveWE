#version 450 core

layout (binding = 0) uniform sampler2DArray water_textures;

layout (location = 6) uniform int current_texture;

in vec2 UV;
in vec4 Color;

out vec4 outColor;

void main() {
	outColor = texture(water_textures, vec3(UV, current_texture)) * Color;
}