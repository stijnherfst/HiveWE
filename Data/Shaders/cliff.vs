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
	pathing_map_uv = (vec2(vPosition.x + 128, vPosition.y) / 128 + vOffset.xy) * 4;
 
	ivec2 size = textureSize(height_texture, 0);
	float value = texture(height_texture, (vOffset.xy + vec2(vPosition.x + 192, vPosition.y + 64) / 128) / vec2(size)).r;

	gl_Position = MVP * vec4(vPosition + vec3(vOffset.xy + vec2(1, 0), vOffset.z + value) * 128, 1);
	UV = vec3(vUV, vOffset.a);

	ivec2 height_pos = ivec2(vOffset.xy + vec2(vPosition.x + 128, vPosition.y) / 128);
	ivec3 off = ivec3(1, 1, 0);
	float hL = texelFetch(height_texture, height_pos - off.xz, 0).r;
	float hR = texelFetch(height_texture, height_pos + off.xz, 0).r;
	float hD = texelFetch(height_texture, height_pos - off.zy, 0).r;
	float hU = texelFetch(height_texture, height_pos + off.zy, 0).r;
	vec3 terrain_normal = normalize(vec3(hL - hR, hD - hU, 2.0));

	Normal = terrain_normal;
}