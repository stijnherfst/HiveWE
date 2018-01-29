#version 450 core

layout (location = 0) in vec2 vPosition;
layout (location = 1) uniform mat4 MVP;
layout (location = 2) uniform vec2 offset;
layout (location = 3) uniform vec2 uv_offset;

layout (binding = 0) uniform sampler2D height_texture;
layout (binding = 1) uniform sampler2D brush;

out vec2 UV;

void main() {
	ivec2 size = textureSize(height_texture, 0);
	ivec2 size_brush = textureSize(brush, 0) / 4;

	vec2 local_pos = vec2(gl_InstanceID % size_brush.x, gl_InstanceID / size_brush.x);
	vec2 pos = offset + vPosition + local_pos;
	vec4 height = texelFetch(height_texture, ivec2(pos), 0);

	gl_Position = MVP * vec4(pos, height.r, 1);

	UV = (vPosition + local_pos - uv_offset * 0.25f) / size_brush;
}