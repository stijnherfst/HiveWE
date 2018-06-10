#version 450 core

layout (binding = 3) uniform sampler2DArray water_textures;
layout (binding = 2) uniform sampler2D water_exists_texture;


layout (location = 5) uniform int current_texture;

in vec2 UV;
in vec4 Color;

out vec4 outColor;

void main() {
	outColor = texture(water_textures, vec3(UV, current_texture)) * Color;
}