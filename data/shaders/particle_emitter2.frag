#version 450 core

in vec2 v_uv;
in vec4 v_color;

layout (binding = 0) uniform sampler2D u_texture;
layout (location = 1) uniform int u_filter_mode;

out vec4 frag_color;

void main() {
	vec4 tex = texture(u_texture, v_uv);
	vec4 c = tex * v_color;
	if (u_filter_mode == 4 && c.a < 0.5) {
		discard;
	}
	frag_color = c;
}
