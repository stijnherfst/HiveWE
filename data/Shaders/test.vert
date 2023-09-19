#version 450 core

layout (location = 0) in vec2 vPosition;

void main() { 
	gl_Position = vec4(vPosition, 1.f, 1.f);
}