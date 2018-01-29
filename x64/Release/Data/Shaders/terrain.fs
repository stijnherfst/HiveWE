#version 450 core

layout (location = 2) uniform bool show_pathing_map_static;
//layout (location = 3) uniform bool show_pathing_map_dynamic;

layout (binding = 0) uniform sampler2DArray ground_textures;
layout (binding = 3) uniform usampler2D pathing_map_static;
//layout (binding = 3) uniform usampler2D pathing_map_dynamic;

in vec2 UV;
in flat uvec4 texture_indices;
in vec2 pathing_map_uv;

out vec4 color;

void main() {
	color = texture(ground_textures, vec3(UV, texture_indices.a));
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.b)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.g)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.r)) * (1 - color.a);

	uvec4 byte = texelFetch(pathing_map_static, ivec2(pathing_map_uv), 0);
	if (show_pathing_map_static) {
		color = color + vec4(min(byte.r & 2, 1), min(byte.r & 4, 1), min(byte.r & 8, 1), 255) * 0.25;
	}

	// byte = texelFetch(pathing_map_dynamic, ivec2(pathing_map_uv), 0);
	// if (show_pathing_map_dynamic) {
	// 	color = color + vec4(min(byte.r & 2, 1), min(byte.r & 4, 1), min(byte.r & 8, 1), 255) * 0.25;
	// }
}