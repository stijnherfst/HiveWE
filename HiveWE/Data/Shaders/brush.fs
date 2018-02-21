#version 330 core

uniform sampler2D brush;

in vec2 UV;

out vec4 color;

void main() {
	color = texture(brush, UV);
}