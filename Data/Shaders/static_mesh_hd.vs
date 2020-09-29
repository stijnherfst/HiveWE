#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;

layout (location = 0) uniform mat4 VP;
layout (location = 3) uniform mat4 M;

out vec2 UV;
out vec3 TangentLightDirection;

void main() {
	gl_Position = VP * M * vec4(vPosition, 1);
	UV = vUV;

	mat3 Matrix = mat3(M);
	vec3 T = normalize(Matrix * vec3(vTangent));
	vec3 N = normalize(Matrix * vNormal);
	vec3 B = cross(N, T);
	mat3 TBN = transpose(mat3(T, B, N));

	vec3 light_direction = normalize(vec3(-0.3f, -0.3f, 0.25f));
	TangentLightDirection = TBN * light_direction;
}