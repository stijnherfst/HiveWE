#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 vPosition;
layout (location = 1) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D height_texture;
layout (binding = 2) uniform usampler2D terrain_texture_list;

layout (location = 0) out vec2 UV;
layout (location = 1) out flat uvec4 texture_indices;
layout (location = 2) out vec2 pathing_map_uv;

void main() { 
	ivec2 size = textureSize(height_texture, 0);
	ivec2 pos = ivec2(gl_InstanceID % size.x, gl_InstanceID / size.x);

	vec4 height = texelFetch(height_texture, ivec2(vPosition + pos), 0);

	UV = vec2(vPosition.x, 1 - vPosition.y);
	texture_indices = texelFetch(terrain_texture_list, pos, 0);
	pathing_map_uv = (vPosition + pos) * 4;	

	// Cliff culling
	gl_Position = ((texture_indices.a & 32768u) == 0u) ? MVP * vec4(vPosition + pos, height.r, 1) : vec4(2.0, 0.0, 0.0, 1.0);
}