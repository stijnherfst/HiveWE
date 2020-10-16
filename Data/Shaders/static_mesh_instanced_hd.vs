#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in mat4 vInstance;
layout (location = 8) in vec3 vGeosetColor;

layout (location = 0) uniform mat4 VP;

out vec2 UV;
out vec3 TangentLightDirection;
out vec3 vertexColor;

void main() {
	gl_Position = VP * vInstance * vec4(vPosition, 1);
	UV = vUV;

	mat3 Matrix = mat3(vInstance);
	vec3 T = normalize(Matrix * vec3(vTangent));
	vec3 N = normalize(Matrix * vNormal);
	vec3 B = cross(N, T);
	mat3 TBN = transpose(mat3(T, B, N));

	vec3 light_direction = normalize(vec3(-0.3f, -0.3f, 0.25f));
	TangentLightDirection = TBN * light_direction;
	vertexColor = vGeosetColor;
}