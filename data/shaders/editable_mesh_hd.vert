#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in uvec4 vSkin;

layout (location = 0) uniform mat4 MVP;
layout (location = 3) uniform vec3 light_direction;
layout (location = 8) uniform vec4 layer_color;
layout (location = 9) uniform int team_color_index;
layout (location = 11) uniform mat4 bones[217];

out vec2 UV;
out vec3 tangent_light_direction;
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
	const mat4 b0 = bones[int(vSkin.x & 0x0000FFFFu)];
	const mat4 b1 = bones[int(vSkin.x >> 16)];
	const mat4 b2 = bones[int(vSkin.y & 0x0000FFFFu)];
	const mat4 b3 = bones[int(vSkin.y >> 16)];
	const float w0 = (vSkin.z & 0x0000FFFFu) / 255.f;
	const float w1 = (vSkin.z >> 16) / 255.f;
	const float w2 = (vSkin.w & 0x0000FFFFu) / 255.f;
	const float w3 = (vSkin.w >> 16) / 255.f;
	const mat4 skin_matrix = b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3;

	gl_Position = MVP * skin_matrix * vec4(vPosition, 1.f);

	mat3 model = mat3(skin_matrix);
	vec3 T = normalize(model * vTangent.xyz);
	vec3 N = normalize(model * vNormal);
	vec3 B = cross(N, T) * vTangent.w;
	mat3 TBN = transpose(mat3(T, B, N));

	UV = vUV;
	tangent_light_direction = normalize(TBN * light_direction);
	vertexColor = layer_color;
	team_color = team_colors[team_color_index];
}
