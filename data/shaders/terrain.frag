#version 450 core

#extension GL_ARB_bindless_texture : require

layout (location = 2) uniform bool show_pathing_map;
layout (location = 3) uniform bool show_lighting;
layout (location = 4) uniform vec3 light_direction;
layout (location = 5) uniform vec2 brush_position;
layout (location = 6) uniform bool render_brush;

layout (binding = 17) uniform usampler2D pathing_map_static;
layout (binding = 18) uniform usampler2D pathing_map_dynamic;
layout (binding = 19) uniform sampler2D brush;

layout (location = 0) in vec2 UV;
layout (location = 1) in flat uvec4 texture_indices;
layout (location = 2) in vec2 pathing_map_uv;
layout (location = 3) in vec3 normal;
layout (location = 4) in vec2 world_position;

layout (location = 0) out vec4 color;

layout(std430, binding = 4) buffer TextureHandles {
	sampler2DArray textures[];
};

vec4 get_fragment(uint id, vec3 uv) {
	if (id == 0xFFFFu) {
		return vec4(0, 0, 0, 0);
	} else {
		return texture(textures[id], uv).rgba;
	}
}

void main() {
	color = get_fragment(texture_indices.a & 0xFFFFu, vec3(UV, texture_indices.a >> 16));
	color = mix(get_fragment(texture_indices.b & 0xFFFFu, vec3(UV, texture_indices.b >> 16)), color, color.a);
	color = mix(get_fragment(texture_indices.g & 0xFFFFu, vec3(UV, texture_indices.g >> 16)), color, color.a);
	color = mix(get_fragment(texture_indices.r & 0xFFFFu, vec3(UV, texture_indices.r >> 16)), color, color.a);

	if (show_lighting) {
		float contribution = (dot(-light_direction, normal) + 1.f) * 0.5f;

		color.rgb *= clamp(contribution, 0.f, 1.f);
	}

	uint byte_static = texelFetch(pathing_map_static, ivec2(pathing_map_uv), 0).r;
	uint byte_dynamic = texelFetch(pathing_map_dynamic, ivec2(pathing_map_uv), 0).r;
	if (show_pathing_map) {
		uint final = byte_static.r | byte_dynamic.r;

		vec3 pathing_color = vec3((final & 2u) >> 1, (final & 4u) >> 2, (final & 8u) >> 3);
		color.rgb = (final & 0xEu) > 0 ? mix(color.rgb, pathing_color, 0.50) : color.rgb;
	}

	if (render_brush) {
		ivec2 brush_texture_size = textureSize(brush, 0);

		vec2 brush_uv = ((brush_position - world_position) * 4.f) / vec2(brush_texture_size) + 0.5;

		vec4 brush_color = texture(brush, brush_uv);

		if (brush_uv.x >= 0.f && brush_uv.y >= 0.f && brush_uv.x <= 1.f && brush_uv.y <= 1.f) {
			color.rgb = mix(color.rgb, brush_color.rgb, brush_color.a);
		}
	} 
}