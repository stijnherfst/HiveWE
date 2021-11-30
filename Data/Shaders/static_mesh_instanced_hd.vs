#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in mat4 vInstance;
layout (location = 8) in vec3 vGeosetColor;

layout (location = 0) uniform mat4 VP;

layout (location = 3) uniform vec3 light_direction;

out vec2 UV;
out vec3 tangent_light_direction;
out vec3 vertexColor;

void main() {
	gl_Position = VP * vInstance * vec4(vPosition, 1.f);
	UV = vUV;

	mat3 model = mat3(vInstance);
	vec3 T = normalize(model * vTangent.xyz);
	vec3 N = normalize(model * vNormal);
	vec3 B = cross(N, T) * vTangent.w; // to fix handedness
	mat3 TBN = transpose(mat3(T, B, N));

	tangent_light_direction = normalize(TBN * light_direction);
	vertexColor = vGeosetColor;
}