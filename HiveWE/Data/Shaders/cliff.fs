#version 450 core

layout (binding = 0) uniform sampler2DArray cliff_textures;
layout (binding = 2) uniform usampler2D pathing_map_static;

layout (location = 4) uniform bool show_pathing_map_static;

layout (location = 0) in vec3 UV;
layout (location = 1) in vec2 pathing_map_uv;

out vec4 color;

void main() {
	color = texture(cliff_textures, UV);

	uvec4 byte = texelFetch(pathing_map_static, ivec2(pathing_map_uv), 0);
	if (show_pathing_map_static) {
		vec4 pathing_color = vec4(min(byte.r & 2, 1), min(byte.r & 4, 1), min(byte.r & 8, 1), 0.25);
		color = length(pathing_color.rgb) > 0 ? color * 0.75 + pathing_color * 0.5 : color;
	}
}