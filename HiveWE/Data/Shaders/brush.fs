#version 450 core

layout (binding = 1) uniform sampler2D brush;

in vec2 UV;

out vec4 color;

void main() {
	//color = vec4(0, 1, 0, 0.25);
	//color = vec4(UV.s, UV.t, 0, 1);
	
	color = texture(brush, UV);
}