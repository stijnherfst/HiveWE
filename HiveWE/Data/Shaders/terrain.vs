#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec2 vPosition;
layout (location = 1) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D height_texture;
layout (binding = 2) uniform usampler2D terrain_texture_list;

out vec2 UV;
out flat uvec4 texture_indices;
out vec2 pathing_map_uv;

void main() { 
	ivec2 size = textureSize(height_texture, 0);
	ivec2 pos = ivec2(gl_InstanceID % size.x, gl_InstanceID / size.x);

	vec4 height = texelFetch(height_texture, ivec2(vPosition + pos), 0);

	UV = vPosition;
	texture_indices = texelFetch(terrain_texture_list, pos, 0);
	pathing_map_uv = (vPosition + pos) * 4;	

	// Cliff culling
	uint flag = 32768u;
	uint text_ind = texture_indices.a;
	flag = text_ind & flag;
	
	if (flag == 0u){
		gl_Position = MVP * vec4(vPosition + pos, height.r, 1);
	} else {
	    gl_Position = vec4(2.0, 0.0, 0.0, 1.0);
	}
}