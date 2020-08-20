#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in mat4 vInstance;

layout (location = 0) uniform mat4 VP;

out vec2 UV;
out vec3 Normal;
out vec3 TangentLightDirection;

void main() {
	gl_Position = VP * vInstance * vec4(vPosition, 1);
	UV = vUV;
	Normal = normalize(mat3(vInstance) * vNormal);

	vec3 T = normalize(vec3(vInstance * vTangent));
	vec3 N = normalize(vec3(vInstance * vec4(vNormal, 0.f)));
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	mat3 invTBN = transpose(TBN);

	vec3 light_direction = vec3(-0.3f, -0.3f, 0.25f);
	light_direction = normalize(light_direction);
	TangentLightDirection = invTBN * light_direction;
}