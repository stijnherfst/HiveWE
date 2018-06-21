#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec4 vOffset;

layout (location = 3) uniform mat4 MVP;

layout (binding = 1) uniform sampler2D ground_height;

layout (location = 0) out vec3 UV;
layout (location = 1) out vec2 pathing_map_uv;

void main() {
	pathing_map_uv = (vec2(vPosition.x + 128, vPosition.y) / 128 + vOffset.xy) * 4;
 
	ivec2 size = textureSize(ground_height, 0);
	float value = texture(ground_height, (vOffset.xy + vec2(vPosition.x + 192, vPosition.y + 64) / 128) / vec2(size)).r;

	gl_Position = MVP * vec4(vPosition + vec3(vOffset.xy + vec2(1, 0), vOffset.z + value) * 128, 1);
	UV = vec3(vUV, vOffset.a);
}