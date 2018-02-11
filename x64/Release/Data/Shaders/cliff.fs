#version 450 core

layout (binding = 0) uniform sampler2D image;

in vec2 UV;

out vec4 outColor;

void main() {
	outColor = texture(image, UV);
	//outColor = vec4(1, 1, 1, 1);
}