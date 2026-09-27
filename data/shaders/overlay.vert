#version 450 core

// Fullscreen triangle for compositing a CPU-drawn (QPainter) overlay over the frame

layout (location = 0) out vec2 uv;

void main() {
	const vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
	// Clip space is Y-up (flipped viewport) while image row 0 is the top
	uv = vec2(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);
	gl_Position = vec4(position, 0.0, 1.0);
}
