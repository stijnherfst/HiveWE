#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec4 vOffset;

layout (location = 3) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D ground_height;

out vec3 UV;

void main() {
	vec4 bottom_left = texelFetch(ground_height, ivec2(vOffset) + ivec2(0, 0), 0);
	vec4 bottom_right = texelFetch(ground_height, ivec2(vOffset) + ivec2(1, 0), 0);
	vec4 top_left = texelFetch(ground_height, ivec2(vOffset) + ivec2(0, 1), 0);
	vec4 top_right = texelFetch(ground_height, ivec2(vOffset) + ivec2(1, 1), 0);

	float bottom = mix(bottom_right.r, bottom_left.r, -(vPosition.x / 128.f));
	float top = mix(top_right.r, top_left.r, -(vPosition.x / 128.f));
	float value = mix(bottom, top, vPosition.y / 128.f);


	gl_Position = MVP * vec4(vPosition + vec3(vOffset.xy + vec2(1, 0), vOffset.z + value) * 128, 1);
	UV = vec3(vUV, vOffset.a);
}