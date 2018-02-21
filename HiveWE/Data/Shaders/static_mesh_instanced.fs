#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

uniform sampler2D image;
layout (location = 3) uniform float alpha_test;

in vec2 UV;

out vec4 outColor;

void main() {
	outColor = texture(image, UV);

	if (outColor.a < alpha_test) {
		discard;
	}
}