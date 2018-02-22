#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 2) uniform bool show_pathing_map_static;
//layout (location = 3) uniform bool show_pathing_map_dynamic;

layout (binding = 0) uniform sampler2DArray ground_textures;
layout (binding = 3) uniform usampler2D pathing_map_static;
//layout (binding = 3) uniform usampler2D pathing_map_dynamic;

layout (location = 0) in vec2 UV;
layout (location = 1) in flat uvec4 texture_indices;
layout (location = 2) in vec2 pathing_map_uv;

out vec4 color;

void main() {
	color = texture(ground_textures, vec3(UV, texture_indices.a));
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.b)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.g)) * (1 - color.a);
	color = color * color.a + texture(ground_textures, vec3(UV, texture_indices.r)) * (1 - color.a);

	uvec4 byte = texelFetch(pathing_map_static, ivec2(pathing_map_uv), 0);
	int xx1 = int(byte.r & 2u);
	int xx2 = int(byte.r & 4u);
	int xx3 = int(byte.r & 8u);
	
	if (show_pathing_map_static) {
		color = color + vec4(min(xx1, 1), min(xx2, 1), min(xx3, 1), 255) * 0.25;
	}

	// byte = texelFetch(pathing_map_dynamic, ivec2(pathing_map_uv), 0);
	// if (show_pathing_map_dynamic) {
	// 	color = color + vec4(min(byte.r & 2, 1), min(byte.r & 4, 1), min(byte.r & 8, 1), 255) * 0.25;
	// }
}