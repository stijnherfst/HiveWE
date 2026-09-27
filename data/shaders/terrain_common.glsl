// Shared by the terrain, cliff and water shaders. Mirrors TerrainFrameData in src/base/terrain.ixx.

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

struct Region {
	vec4 rect; // left, bottom, right, top
	vec4 color; // rgb + selected flag
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer FloatBuffer {
	float values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer UintBuffer {
	uint values[];
};

// Tightly packed vertex streams
layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec2Buffer {
	vec2 values[];
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec3Buffer {
	vec3 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Vec4Buffer {
	vec4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Uvec4Buffer {
	uvec4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer RegionBuffer {
	Region values[];
};

const uint flag_pathing = 1u;
const uint flag_lighting = 2u;
const uint flag_regions = 4u;
const uint flag_brush = 8u;

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer TerrainFrame {
	mat4 mvp;
	vec4 light_direction;
	vec4 shallow_color_min;
	vec4 shallow_color_max;
	vec4 deep_color_min;
	vec4 deep_color_max;
	vec2 brush_position;
	ivec2 map_size;
	/// Final corner heights including the layer (cliff) height
	FloatBuffer cliff_levels;
	/// Ground heights without the layer height, for normals
	FloatBuffer ground_heights;
	/// Per tile: up to four texture ids with variation/blend masks in the upper 16 bits
	Uvec4Buffer ground_textures;
	UintBuffer ground_exists;
	/// Bindless slot of each ground texture id
	UintBuffer ground_texture_slots;
	FloatBuffer water_heights;
	UintBuffer water_exists;
	RegionBuffer regions;
	uint region_count;
	uint flags;
	uint pathing_static_slot;
	uint pathing_dynamic_slot;
	uint brush_slot;
	uint cliff_texture_slot;
	uint water_texture_slot;
	float water_offset;
	int current_water_texture;
};

// The ground and water draws push only frame. Mirrors CliffPushConstants in src/resources/cliff_mesh.ixx.
layout(push_constant, std430) uniform PushConstants {
	TerrainFrame frame;
	/// The cliff mesh's vertex streams and the draw's instances; only used by the cliff shader
	Vec3Buffer cliff_positions;
	Vec2Buffer cliff_uvs;
	Vec3Buffer cliff_normals;
	/// Per instance: tile x, tile y, base layer height, cliff texture index
	Vec4Buffer cliff_instances;
} pc;

// Views of the one bindless binding; each slot holds a view of the matching type
layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 0) uniform sampler2DArray array_textures[];
layout(set = 0, binding = 0) uniform usampler2D uint_textures[];

vec3 apply_regions(TerrainFrame frame, vec3 color, vec2 p) {
	// The border/corner sizes must match the RegionBrush grab zones
	const float border = 0.1;
	const float corner = 0.20;
	const float outline = 0.02;

	for (uint i = 0u; i < frame.region_count; i++) {
		const vec4 rect = frame.regions.values[i].rect;
		if (p.x < rect.x || p.x > rect.z || p.y < rect.y || p.y > rect.w) {
			continue;
		}

		const vec3 region_color = frame.regions.values[i].color.rgb;
		const bool selected = frame.regions.values[i].color.a > 0.5;

		vec3 border_color = region_color;
		if (selected) {
			const float luminance = dot(region_color, vec3(0.299, 0.587, 0.114));
			border_color = luminance > 0.65 ? region_color * 0.45 : mix(region_color, vec3(1.0), 0.6);
		}

		// Distance to the nearest vertical/horizontal region edge
		const float dx = min(p.x - rect.x, rect.z - p.x);
		const float dy = min(p.y - rect.y, rect.w - p.y);

		if (selected && dx < corner && dy < corner) {
			const bool corner_outline = dx > corner - outline || dy > corner - outline;
			color = corner_outline ? vec3(0.0) : border_color;
		} else if (dx < border || dy < border) {
			color = border_color;
		} else {
			color = mix(color, region_color, 0.25);
		}
	}

	return color;
}
