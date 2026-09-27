#version 450 core

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 color;

void main() {
	const float R = 1.0;
	const float R2 = 0.93;
	const float dist = sqrt(dot(uv * 2.f - 1.f, uv * 2.f - 1.f));
	if (dist >= R || dist <= R2) {
		discard;
	}
	color = vec4(0, 1, 0, 0.75);
}
