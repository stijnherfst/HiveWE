#version 450 core

layout (location = 0) in vec2 vPosition;
layout (location = 1) uniform mat4 MVP;

layout (binding = 0) uniform sampler2D height_texture;
layout (binding = 1) uniform sampler2D height_cliff_texture;
layout (binding = 2) uniform usampler2D terrain_texture_list;
layout (binding = 22) uniform sampler2D terrain_exists_texture;

layout (location = 0) out vec2 UV;
layout (location = 1) out flat uvec4 texture_indices;
layout (location = 2) out vec2 pathing_map_uv;
layout (location = 3) out vec3 normal;

void main() { 
	const ivec2 size = textureSize(terrain_texture_list, 0);
	const ivec2 pos = ivec2(gl_InstanceID % size.x, gl_InstanceID / size.x);

	const ivec2 height_pos = ivec2(vPosition + pos);
	const vec4 height = texelFetch(height_cliff_texture, height_pos, 0);

	const ivec3 offset = ivec3(1, 1, 0);
	const float hL = texelFetch(height_texture, height_pos - offset.xz, 0).r;
	const float hR = texelFetch(height_texture, height_pos + offset.xz, 0).r;
	const float hD = texelFetch(height_texture, height_pos - offset.zy, 0).r;
	const float hU = texelFetch(height_texture, height_pos + offset.zy, 0).r;
	normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	UV = vec2(vPosition.x, 1 - vPosition.y);
	texture_indices = texelFetch(terrain_texture_list, pos, 0);
	pathing_map_uv = (vPosition + pos) * 4;	

	const ivec2 pos2 = ivec2(gl_InstanceID % (size.x + 1), gl_InstanceID / (size.x + 1));
	const bool is_ground = texelFetch(terrain_exists_texture, pos2, 0).r > 0;

	gl_Position = is_ground ? MVP * vec4(vPosition + pos, height.r, 1) : vec4(2.0, 0.0, 0.0, 1.0);
}