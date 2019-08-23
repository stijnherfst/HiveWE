#version 450 core

layout (binding = 1) uniform sampler2D brush;

in vec2 UV;

out vec4 color;

void main() {
	if (UV.x < 0.f || UV.y < 0.f || UV.x > 1.f || UV.y > 1.f) {
		color = vec4(0, 0, 0, 0);
	} else {
		color = texture(brush, UV);
	}
}