#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) out vec2 UV;
layout (location = 1) out flat uvec4 texture_indices;
layout (location = 2) out vec2 pathing_map_uv;
layout (location = 3) out vec3 normal;
layout (location = 4) out vec2 world_position;

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
	const ivec2 pos = ivec2(gl_InstanceIndex % (map_size.x - 1), gl_InstanceIndex / (map_size.x - 1));

	const vec2 vPosition = position[gl_VertexIndex];
	const ivec2 height_pos = ivec2(vPosition + pos);
	const float height = frame.cliff_levels.values[height_pos.y * map_size.x + height_pos.x];

	const float hL = frame.ground_heights.values[height_pos.y * map_size.x + max(height_pos.x - 1, 0)];
	const float hR = frame.ground_heights.values[height_pos.y * map_size.x + min(height_pos.x + 1, map_size.x)];
	const float hD = frame.ground_heights.values[max(height_pos.y - 1, 0) * map_size.x + height_pos.x];
	const float hU = frame.ground_heights.values[min(height_pos.y + 1, map_size.y) * map_size.x + height_pos.x];
	normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	UV = vec2(vPosition.x, 1 - vPosition.y);
	texture_indices = frame.ground_textures.values[pos.y * (map_size.x - 1) + pos.x];
	pathing_map_uv = (vPosition + pos) * 4;

	// One entry per tile, in instance order
	const bool is_ground = frame.ground_exists.values[gl_InstanceIndex] > 0u;

	gl_Position = is_ground ? frame.mvp * vec4(vPosition + pos, height, 1) : vec4(2.0, 0.0, 0.0, 1.0);
	world_position = vPosition + pos;
}
