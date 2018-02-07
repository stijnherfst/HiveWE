#version 450 core

layout (binding = 1) uniform sampler2D brush;

in vec2 UV;

out vec4 color;

void main() {
	color = texture(brush, UV);
}