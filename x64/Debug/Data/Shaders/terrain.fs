#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout (location = 2) uniform bool show_pathing_map_static;
//layout (location = 3) uniform bool show_pathing_map_dynamic;

uniform sampler2DArray ground_textures;
uniform usampler2D pathing_map_static;
//uniform usampler2D pathing_map_dynamic;

in vec2 UV;
flat in uvec4 texture_indices;
in vec2 pathing_map_uv;

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