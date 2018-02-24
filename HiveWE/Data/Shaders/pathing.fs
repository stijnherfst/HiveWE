#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable

in vec3 Color;

out vec4 outColor;

void main() {
	outColor = vec4(Color, 0.25);
}