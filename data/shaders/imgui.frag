#version 450 core

#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant, std430) uniform PushConstants {
	vec2 scale;
	vec2 translate;
	uint texture_slot;
} pc;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout (location = 0) in vec2 uv;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = color * texture(textures[pc.texture_slot], uv);
}
