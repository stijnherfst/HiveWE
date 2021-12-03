#version 450 core

out vec4 color;

in vec2 uv;

void main() {
	float R = 1.0; 
	float R2 = 0.93; 
	float dist = sqrt(dot(uv * 2.f - 1.f, uv * 2.f - 1.f));
	if (dist >= R || dist <= R2)
	{ discard; } 
	color = vec4(0, 1, 0, 0.75);
}