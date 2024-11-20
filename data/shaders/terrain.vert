#version 450 core

layout (location = 1) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D height_cliff_texture;

layout (location = 0) out vec2 UV;
layout (location = 1) out flat uvec4 texture_indices;
layout (location = 2) out vec2 pathing_map_uv;
layout (location = 3) out vec3 normal;
layout (location = 4) out vec2 world_position;

layout (location = 7) uniform ivec2 map_size;

const vec2[6] position = vec2[6](
	vec2(1, 1),
	vec2(0, 1),
	vec2(0, 0),
	vec2(0, 0),
	vec2(1, 0),
	vec2(1, 1)
);

layout(std430, binding = 0) buffer layoutName {
    float clif_levels[];
};

layout(std430, binding = 1) buffer layoutName1 {
    float ground_heights[];
};

layout(std430, binding = 2) buffer layoutName2 {
    uvec4 terrain_texture_list[];
};

layout(std430, binding = 3) buffer layoutName3 {
    unsigned char terrain_exists[];
};


void main() { 
	const ivec2 pos = ivec2(gl_InstanceID % (map_size.x - 1), gl_InstanceID / (map_size.x - 1));

	vec2 vPosition = position[gl_VertexID];
	const ivec2 height_pos = ivec2(vPosition + pos);
	const float height = clif_levels[height_pos.y * map_size.x + height_pos.x];

	const ivec3 offset = ivec3(1, 1, 0);
	const float hL = ground_heights[height_pos.y * map_size.x + max(height_pos.x - 1, 0)];
	const float hR = ground_heights[height_pos.y * map_size.x + min(height_pos.x + 1, map_size.x)];
	const float hD = ground_heights[max(height_pos.y - 1, 0) * map_size.x + height_pos.x];
	const float hU = ground_heights[min(height_pos.y + 1, map_size.y) * map_size.x + height_pos.x];
	normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	UV = vec2(vPosition.x, 1 - vPosition.y);
	texture_indices = terrain_texture_list[pos.y * (map_size.x - 1) + pos.x];
	pathing_map_uv = (vPosition + pos) * 4;	

	const ivec2 pos2 = ivec2(gl_InstanceID % map_size.x, gl_InstanceID / map_size.x);
	const bool is_ground = terrain_exists[pos2.y * map_size.x + pos2.x] > 0u;

	gl_Position = is_ground ? MVP * vec4(vPosition + pos, height, 1) : vec4(2.0, 0.0, 0.0, 1.0);
	world_position = vPosition + pos;
}