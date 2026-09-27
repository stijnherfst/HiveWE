#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) in vec3 UV;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 pathing_map_uv;
layout (location = 3) in vec2 world_position;

layout (location = 0) out vec4 color;

void main() {
	TerrainFrame frame = pc.frame;

	color = texture(array_textures[frame.cliff_texture_slot], UV);

	if ((frame.flags & flag_lighting) != 0u) {
		color.rgb *= (dot(-frame.light_direction.xyz, Normal) + 1.f) * 0.5f;
	}

	if ((frame.flags & flag_pathing) != 0u) {
		const uint final = texelFetch(uint_textures[frame.pathing_static_slot], ivec2(pathing_map_uv), 0).r;
		const vec3 pathing_static_color = vec3((final & 2u) >> 1, (final & 4u) >> 2, (final & 8u) >> 3);
		color.rgb = (final & 0xEu) > 0 ? color.rgb * 0.75f + pathing_static_color * 0.5f : color.rgb;
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
