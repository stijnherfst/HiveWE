#version 450 core

layout (binding = 0) uniform sampler2DArray texture;

in vec3 UV;

out vec4 outColor;

void main() {
	outColor = texture(texture, UV);
}