#version 450 core

layout (location = 1) uniform float alpha_test;

out vec4 outColor;

layout (location = 7) uniform int color_id;

void main() {
	outColor = vec4((color_id & 0xFF) / 255.f, ((color_id & 0xFF00) >> 8) / 255.f, ((color_id & 0xFF0000) >> 16) / 255.f, 1.f);
	//outColor = vec4(color_id * 0.5f, 0.f, 1.f, 1.f);
	// if (outColor.a < alpha_test) {
	// 	discard;
	// }
}