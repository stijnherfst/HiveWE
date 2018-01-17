#version 450 core

layout (binding = 0) uniform sampler2DArray textureArray;

in vec2 UV;
in flat uvec4 texture_indices;

out vec4 color;

void main() {
	color = texture(textureArray, vec3(UV, texture_indices.a));
	color = color * color.a + texture(textureArray, vec3(UV, texture_indices.b)) * (1 - color.a);
	color = color * color.a + texture(textureArray, vec3(UV, texture_indices.g)) * (1 - color.a);
	color = color * color.a + texture(textureArray, vec3(UV, texture_indices.r)) * (1 - color.a);
}

