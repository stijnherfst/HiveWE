#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in uvec2 vSkin;

layout (location = 0) uniform mat4 MVP;
layout (location = 4) uniform mat4 model;
layout (location = 6) uniform int bone_count;
layout (location = 8) uniform vec4 layer_color;
layout (location = 9) uniform int team_color_index;
layout (location = 11) uniform mat4 bones[217]; // Thats a lotta boners. max 200 because the shader compiler issues an error when going higher

out vec2 UV;
out vec3 Normal;
out vec4 vertexColor;
out vec3 team_color;

const vec3 team_colors[28] = {
	vec3(1.000, 0.012, 0.012),
	vec3(0.000, 0.259, 1.000),
	vec3(0.106, 0.906, 0.729),
	vec3(0.333, 0.000, 0.506),
	vec3(0.996, 0.988, 0.000),
	vec3(0.996, 0.537, 0.051),
	vec3(0.129, 0.749, 0.000),
	vec3(0.894, 0.361, 0.686),
	vec3(0.576, 0.584, 0.588),
	vec3(0.494, 0.749, 0.945),
	vec3(0.063, 0.384, 0.278),
	vec3(0.310, 0.169, 0.020),
	vec3(0.612, 0.000, 0.000),
	vec3(0.000, 0.000, 0.765),
	vec3(0.000, 0.922, 1.000),
	vec3(0.741, 0.000, 1.000),
	vec3(0.925, 0.808, 0.529),
	vec3(0.969, 0.647, 0.545),
	vec3(0.749, 1.000, 0.506),
	vec3(0.859, 0.722, 0.922),
	vec3(0.310, 0.314, 0.333),
	vec3(0.925, 0.941, 1.000),
	vec3(0.000, 0.471, 0.118),
	vec3(0.647, 0.435, 0.204),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
	vec3(0.180, 0.176, 0.180),
};

void main() {
	const mat4 b0 = bones[int(vSkin.x & 0x000000FFu)];
	const mat4 b1 = bones[int(vSkin.x & 0x0000FF00u) >> 8];
	const mat4 b2 = bones[int(vSkin.x & 0x00FF0000u) >> 16];
	const mat4 b3 = bones[int(vSkin.x & 0xFF000000u) >> 24];
	const float w0 = (vSkin.y & 0x000000FFu) / 255.f;
	const float w1 = ((vSkin.y & 0x0000FF00u) >> 8) / 255.f;
	const float w2 = ((vSkin.y & 0x00FF0000u) >> 16) / 255.f;
	const float w3 = ((vSkin.y & 0xFF000000u) >> 24) / 255.f;
	const mat4 skinMatrix = model * (b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3);
	
	gl_Position = MVP * skinMatrix * vec4(vPosition, 1.f);

	UV = vUV;
	Normal = vNormal;
	vertexColor = layer_color;
	team_color = team_colors[team_color_index];
}