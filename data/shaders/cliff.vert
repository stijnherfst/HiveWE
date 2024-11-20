#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vOffset;

layout (location = 0) uniform mat4 VP;
layout (location = 7) uniform ivec2 map_size;

layout(std430, binding = 0) buffer layoutName1 {
    float cliff_levels[];
};

layout (location = 0) out vec3 UV;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec2 pathing_map_uv;
layout (location = 3) out vec2 world_position;

void main() {
	// WC3 cliff meshes seem to be rotated by 90 degrees so we unrotate
	const vec3 rotated_world_position = vec3(vPosition.y, -vPosition.x, vPosition.z) / 128.f + vOffset.xyz;

	const ivec2 height_pos = ivec2(rotated_world_position.xy);
	const float height = cliff_levels[height_pos.y * map_size.x + height_pos.x];

	const ivec3 off = ivec3(1, 1, 0);

	const float hL = cliff_levels[height_pos.y * map_size.x + max(height_pos.x - 1, 0)];
	const float hR = cliff_levels[height_pos.y * map_size.x + min(height_pos.x + 1, map_size.x)];
	const float hD = cliff_levels[max(height_pos.y - 1, 0) * map_size.x + height_pos.x];
	const float hU = cliff_levels[min(height_pos.y + 1, map_size.y) * map_size.x + height_pos.x];
	const vec3 terrain_normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	gl_Position = VP * vec4(rotated_world_position.xy, rotated_world_position.z + height, 1);

	pathing_map_uv = rotated_world_position.xy * 4;
	UV = vec3(vUV, vOffset.a);

	const vec3 rotated_normal = vec3(vNormal.y, -vNormal.x, vNormal.z);
	Normal = normalize(vec3(rotated_normal.xy + terrain_normal.xy, rotated_normal.z * terrain_normal.z));
	world_position = rotated_world_position.xy;
}