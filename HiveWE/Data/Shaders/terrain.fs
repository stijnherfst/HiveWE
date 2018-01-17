#version 450 core

layout (location = 2) uniform bool show_pathing_map;

layout (binding = 0) uniform sampler2DArray ground_textures;
layout (binding = 3) uniform usampler2D pathing_map;

in vec2 UV;
in flat uvec4 texture_indices;
in vec2 pathing_map_uv;

out vec4 color;

void main() {
	color = texture(ground_textures, vec3(UV, texture_indices.a));
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.b)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.g)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.r)) * (1 - color.a);

	// Pathing map
	if (show_pathing_map) {
		uvec4 byte = texelFetch(pathing_map, ivec2(pathing_map_uv), 0);
		color = color + vec4(min(byte.r & 2, 1), min(byte.r & 4, 1), min(byte.r & 8, 1), 255) * 0.25;
	}
}