#version 450 core

layout (location = 1) uniform float alpha_test;

out vec4 color;

layout (location = 7) uniform int color_id;

void main() {
	color = vec4((color_id & 0xFF) / 255.f, ((color_id & 0xFF00) >> 8) / 255.f, ((color_id & 0xFF0000) >> 16) / 255.f, 1.f);
}