#version 450 core

#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant, std430) uniform PushConstants {
	uint texture_slot;
} pc;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 color;

void main() {
	// Premultiplied alpha, blended with ONE, ONE_MINUS_SRC_ALPHA
	color = texture(textures[pc.texture_slot], uv);
}
