#version 330 core

in vec3 Color;

out vec4 outColor;

void main() {
	outColor = vec4(Color, 0.25);
}