#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) out vec3 UV;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec2 pathing_map_uv;
layout (location = 3) out vec2 world_position;

void main() {
	TerrainFrame frame = pc.frame;
	const ivec2 map_size = frame.map_size;
	const vec3 vPosition = pc.cliff_positions.values[gl_VertexIndex];
	const vec2 vUV = pc.cliff_uvs.values[gl_VertexIndex];
	const vec3 vNormal = pc.cliff_normals.values[gl_VertexIndex];
	const vec4 vOffset = pc.cliff_instances.values[gl_InstanceIndex];

	// WC3 cliff meshes seem to be rotated by 90 degrees so we unrotate
	const vec3 rotated_world_position = vec3(vPosition.y, -vPosition.x, vPosition.z) / 128.f + vOffset.xyz;

	const ivec2 height_pos = ivec2(rotated_world_position.xy);
	const float height = frame.ground_heights.values[height_pos.y * map_size.x + height_pos.x];

	const float hL = frame.ground_heights.values[height_pos.y * map_size.x + max(height_pos.x - 1, 0)];
	const float hR = frame.ground_heights.values[height_pos.y * map_size.x + min(height_pos.x + 1, map_size.x)];
	const float hD = frame.ground_heights.values[max(height_pos.y - 1, 0) * map_size.x + height_pos.x];
	const float hU = frame.ground_heights.values[min(height_pos.y + 1, map_size.y) * map_size.x + height_pos.x];
	const vec3 terrain_normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	gl_Position = frame.mvp * vec4(rotated_world_position.xy, rotated_world_position.z + height, 1);

	pathing_map_uv = rotated_world_position.xy * 4;
	UV = vec3(vUV, vOffset.a);

	const vec3 rotated_normal = vec3(vNormal.y, -vNormal.x, vNormal.z);
	Normal = normalize(vec3(rotated_normal.xy + terrain_normal.xy, rotated_normal.z * terrain_normal.z));
	world_position = rotated_world_position.xy;
}
