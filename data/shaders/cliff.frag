#version 450 core

layout (binding = 0) uniform sampler2DArray cliff_textures;
layout (binding = 2) uniform usampler2D pathing_map_static;
layout (binding = 3) uniform sampler2D brush;

layout (location = 1) uniform bool show_pathing_map_static;
layout (location = 2) uniform bool show_lighting;
layout (location = 3) uniform vec3 light_direction;
layout (location = 4) uniform vec2 brush_position;
layout (location = 5) uniform bool render_brush;

layout (location = 0) in vec3 UV;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 pathing_map_uv;
layout (location = 3) in vec2 world_position;

out vec4 color;

void main() {
	color = texture(cliff_textures, UV);

	if (show_lighting) {
		color.rgb *= (dot(-light_direction, Normal) + 1.f) * 0.5f;
	}

	uvec4 byte = texelFetch(pathing_map_static, ivec2(pathing_map_uv), 0);
	if (show_pathing_map_static) {
		uint final = byte.r;

		vec3 pathing_static_color = vec3((final & 2) >> 1, (final & 4) >> 2, (final & 8) >> 3);
		color.rgb = (final & 0xE) > 0 ? color.rgb * 0.75f + pathing_static_color * 0.5f : color.rgb;
	}

	if (render_brush) {
		ivec2 brush_texture_size = textureSize(brush, 0);

		vec2 extra_offset = vec2(0.0f);
		if (brush_texture_size.x % 4 != 0) {
			extra_offset.x = 0.25f;
		}
		if (brush_texture_size.y % 4 != 0) {
			extra_offset.y = 0.25f;
		}

		vec2 brush_uv = ((brush_position - world_position) * 4.f) / vec2(brush_texture_size) + 0.5;

		vec4 brush_color = texture(brush, brush_uv);

		if (brush_uv.x >= 0.f && brush_uv.y >= 0.f && brush_uv.x <= 1.f && brush_uv.y <= 1.f) {
			color.rgb = mix(color.rgb, brush_color.rgb, brush_color.a);
		}
	} 
}