#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) in vec2 UV;
layout (location = 1) in flat uvec4 texture_indices;
layout (location = 2) in vec2 pathing_map_uv;
layout (location = 3) in vec3 normal;
layout (location = 4) in vec2 world_position;

layout (location = 0) out vec4 color;

vec4 get_fragment(TerrainFrame frame, uint id, vec3 uv) {
	if (id == 0xFFFFu) {
		return vec4(0, 0, 0, 0);
	}
	const uint slot = frame.ground_texture_slots.values[id];
	return texture(array_textures[nonuniformEXT(slot)], uv).rgba;
}

void main() {
	TerrainFrame frame = pc.frame;

	color = get_fragment(frame, texture_indices.a & 0xFFFFu, vec3(UV, texture_indices.a >> 16));
	color = mix(get_fragment(frame, texture_indices.b & 0xFFFFu, vec3(UV, texture_indices.b >> 16)), color, color.a);
	color = mix(get_fragment(frame, texture_indices.g & 0xFFFFu, vec3(UV, texture_indices.g >> 16)), color, color.a);
	color = mix(get_fragment(frame, texture_indices.r & 0xFFFFu, vec3(UV, texture_indices.r >> 16)), color, color.a);

	if ((frame.flags & flag_lighting) != 0u) {
		const float contribution = (dot(-frame.light_direction.xyz, normal) + 1.f) * 0.5f;
		color.rgb *= clamp(contribution, 0.f, 1.f);
	}

	if ((frame.flags & flag_pathing) != 0u) {
		const uint byte_static = texelFetch(uint_textures[frame.pathing_static_slot], ivec2(pathing_map_uv), 0).r;
		const uint byte_dynamic = texelFetch(uint_textures[frame.pathing_dynamic_slot], ivec2(pathing_map_uv), 0).r;
		const uint final = byte_static | byte_dynamic;

		const vec3 pathing_color = vec3((final & 2u) >> 1, (final & 4u) >> 2, (final & 8u) >> 3);
		color.rgb = (final & 0xEu) > 0 ? mix(color.rgb, pathing_color, 0.50) : color.rgb;
	}

	if ((frame.flags & flag_regions) != 0u) {
		color.rgb = apply_regions(frame, color.rgb, world_position);
	}

	if ((frame.flags & flag_brush) != 0u) {
		const ivec2 brush_texture_size = textureSize(textures[frame.brush_slot], 0);
		const vec2 brush_uv = ((frame.brush_position - world_position) * 4.f) / vec2(brush_texture_size) + 0.5;
		const vec4 brush_color = texture(textures[frame.brush_slot], brush_uv);

		if (brush_uv.x >= 0.f && brush_uv.y >= 0.f && brush_uv.x <= 1.f && brush_uv.y <= 1.f) {
			color.rgb = mix(color.rgb, brush_color.rgb, brush_color.a);
		}
	}
}
