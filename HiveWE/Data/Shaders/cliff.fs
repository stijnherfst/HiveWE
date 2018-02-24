#version 330
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2DArray cliff_textures;

in vec3 UV;

out vec4 outColor;

void main() {
	outColor = texture(cliff_textures, UV);
}