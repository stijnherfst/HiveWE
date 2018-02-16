#version 450 core

layout (binding = 0) uniform sampler2DArray cliff_textures;

in vec3 UV;

out vec4 outColor;

void main() {
	outColor = texture(cliff_textures, UV);
}