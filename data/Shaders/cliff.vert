#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vOffset;

layout (location = 0) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D height_texture;

layout (location = 0) out vec3 UV;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec2 pathing_map_uv;

void main() {
	// WC3 cliff meshes seem to be rotated by 90 degrees so we unrotate
	const vec3 rotated_world_position = vec3(vPosition.y, -vPosition.x, vPosition.z) / 128.f + vOffset.xyz;

	const ivec2 size = textureSize(height_texture, 0);
	const float height = texture(height_texture, (rotated_world_position.xy + 0.5f) / vec2(size)).r;

	const ivec2 height_pos = ivec2(rotated_world_position.xy);
	const ivec3 off = ivec3(1, 1, 0);
	const float hL = texelFetch(height_texture, height_pos - off.xz, 0).r;
	const float hR = texelFetch(height_texture, height_pos + off.xz, 0).r;
	const float hD = texelFetch(height_texture, height_pos - off.zy, 0).r;
	const float hU = texelFetch(height_texture, height_pos + off.zy, 0).r;
	const vec3 terrain_normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	gl_Position = MVP * vec4(rotated_world_position.xy, rotated_world_position.z + height, 1);

	pathing_map_uv = rotated_world_position.xy * 4;
	UV = vec3(vUV, vOffset.a);

	const vec3 rotated_normal = vec3(vNormal.y, -vNormal.x, vNormal.z);
	Normal = normalize(vec3(rotated_normal.xy + terrain_normal.xy, rotated_normal.z * terrain_normal.z));
}