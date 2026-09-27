#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) out vec2 UV;
layout (location = 1) out vec4 Color;

const float min_depth = 10.f / 128.f;
const float deeplevel = 64.f / 128.f;
const float maxdepth = 72.f / 128.f;

const vec2[6] position = vec2[6](
	vec2(1, 1),
	vec2(0, 1),
	vec2(0, 0),
	vec2(0, 0),
	vec2(1, 0),
	vec2(1, 1)
);

void main() {
	TerrainFrame frame = pc.frame;
	const ivec2 map_size = frame.map_size;

	// Position of the quad's bottom left vertex
	const ivec2 quad_pos = ivec2(gl_InstanceIndex % (map_size.x - 1), gl_InstanceIndex / (map_size.x - 1));

	const bool is_water = frame.water_exists.values[quad_pos.y * map_size.x + quad_pos.x] > 0u
		|| frame.water_exists.values[quad_pos.y * map_size.x + quad_pos.x + 1] > 0u
		|| frame.water_exists.values[(quad_pos.y + 1) * map_size.x + quad_pos.x] > 0u
		|| frame.water_exists.values[(quad_pos.y + 1) * map_size.x + quad_pos.x + 1] > 0u;

	UV = vec2(position[gl_VertexIndex].x, 1.f - position[gl_VertexIndex].y);

	const ivec2 vertex_world_pos = ivec2(position[gl_VertexIndex]) + quad_pos;
	const float water_height = frame.water_heights.values[vertex_world_pos.y * map_size.x + vertex_world_pos.x] + frame.water_offset;
	const float ground_height = frame.cliff_levels.values[vertex_world_pos.y * map_size.x + vertex_world_pos.x];

	float value = clamp(water_height - ground_height, 0.f, 1.f);
	if (value <= deeplevel) {
		value = max(0.f, value - min_depth) / (deeplevel - min_depth);
		Color = frame.shallow_color_min * (1.f - value) + frame.shallow_color_max * value;
	} else {
		value = clamp(value - deeplevel, 0.f, maxdepth - deeplevel) / (maxdepth - deeplevel);
		Color = frame.deep_color_min * (1.f - value) + frame.deep_color_max * value;
	}

	gl_Position = is_water ? frame.mvp * vec4(vertex_world_pos, water_height, 1) : vec4(2.0, 0.0, 0.0, 1.0);
}
