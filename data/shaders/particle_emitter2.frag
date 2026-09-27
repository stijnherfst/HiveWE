#version 450 core

#extension GL_EXT_nonuniform_qualifier : require

// Mirrors ParticlePushConstants in vk_editable_mesh.ixx
layout(push_constant, std430) uniform PushConstants {
	mat4 mvp;
	uint texture_slot;
	int filter_mode;
} pc;

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 frag_color;

void main() {
	vec4 tex = texture(textures[pc.texture_slot], v_uv);
	vec4 c = tex * v_color;
	if (pc.filter_mode == 4 && c.a < 0.5) {
		discard;
	}
	frag_color = c;
}
