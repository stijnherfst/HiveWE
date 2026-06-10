#version 450 core

layout (binding = 0) uniform sampler2DArray cliff_textures;
layout (binding = 2) uniform usampler2D pathing_map_static;
layout (binding = 3) uniform sampler2D brush;

layout (location = 1) uniform bool show_pathing_map_static;
layout (location = 2) uniform bool show_lighting;
layout (location = 3) uniform vec3 light_direction;
layout (location = 4) uniform vec2 brush_position;
layout (location = 5) uniform bool render_brush;
layout (location = 8) uniform bool show_regions;
layout (location = 9) uniform uint region_count;

layout (location = 0) in vec3 UV;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 pathing_map_uv;
layout (location = 3) in vec2 world_position;

out vec4 color;

struct Region {
	vec4 rect; // left, bottom, right, top
	vec4 color; // rgb + selected flag
};

layout(std430, binding = 5) buffer RegionData {
	Region regions[];
};

vec3 apply_regions(vec3 color, vec2 p) {
	// The border/corner sizes must match the RegionBrush grab zones
	const float border = 0.1;
	const float corner = 0.20;
	const float outline = 0.02;

	for (uint i = 0u; i < region_count; i++) {
		vec4 rect = regions[i].rect;
		if (p.x < rect.x || p.x > rect.z || p.y < rect.y || p.y > rect.w) {
			continue;
		}

		vec3 region_color = regions[i].color.rgb;
		bool selected = regions[i].color.a > 0.5;

		vec3 border_color = region_color;
		if (selected) {
			const float luminance = dot(region_color, vec3(0.299, 0.587, 0.114));
			border_color = luminance > 0.65 ? region_color * 0.45 : mix(region_color, vec3(1.0), 0.6);
		}

		// Distance to the nearest vertical/horizontal region edge
		const float dx = min(p.x - rect.x, rect.z - p.x);
		const float dy = min(p.y - rect.y, rect.w - p.y);

		if (selected && dx < corner && dy < corner) {
			bool corner_outline = dx > corner - outline || dy > corner - outline;
			color = corner_outline ? vec3(0.0) : border_color;
		} else if (dx < border || dy < border) {
			color = border_color;
		} else {
			color = mix(color, region_color, 0.25);
		}
	}

	return color;
}

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

	if (show_regions) {
		color.rgb = apply_regions(color.rgb, world_position);
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