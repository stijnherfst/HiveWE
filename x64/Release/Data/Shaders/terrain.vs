#version 450 core

layout (location = 0) in vec2 vPosition;
layout (location = 2) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D height_texture;
layout (binding = 2) uniform usampler2D terrain_texture_list;

out vec2 UV;
out flat uvec4 texture_indices;

void main() { 
	ivec2 size = textureSize(height_texture, 0);

	vec2 pos = vec2(vPosition.x + gl_InstanceID % size.x, vPosition.y + gl_InstanceID / size.y);
	vec4 height = texelFetch(height_texture, ivec2(pos), 0);
	gl_Position = MVP * vec4(pos, height.r, 1 );
	UV = vPosition;


	ivec2 pos2 = ivec2(gl_InstanceID % size.x, gl_InstanceID / size.y);
	texture_indices = texelFetch(terrain_texture_list, ivec2(pos2), 0);
}