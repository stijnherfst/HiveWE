#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable


layout (binding = 1) uniform sampler2D brush;

in vec2 UV;

out vec4 color;

void main() {
	color = texture(brush, UV);
}