#version 420 core

layout (binding = 0) uniform sampler2DArray textureArray;

in vec3 UVW;

out vec4 outColor;

void main() {
	outColor = texture(textureArray, UVW);
}