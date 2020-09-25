#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in uvec2 vSkin;
layout (location = 5) in mat4 vInstance;
layout (location = 9) in float layer_alpha;
layout (location = 10) in vec3 geoset_color;

layout (location = 0) uniform mat4 VP;
layout (location = 3) uniform int nodeCount;

layout (binding = 2) uniform samplerBuffer nodeMatrices;

out vec2 UV;
out vec3 TangentLightDirection;
out vec4 vertexColor;

mat4 fetchMatrix(int nodeIndex) {
	return mat4(
		texelFetch(nodeMatrices, gl_InstanceID * nodeCount * 4 + nodeIndex * 4),
		texelFetch(nodeMatrices, gl_InstanceID * nodeCount * 4 + nodeIndex * 4 + 1),
		texelFetch(nodeMatrices, gl_InstanceID * nodeCount * 4 + nodeIndex * 4 + 2),
		texelFetch(nodeMatrices, gl_InstanceID * nodeCount * 4 + nodeIndex * 4 + 3));
}

void main() {
	const mat4 b0 = fetchMatrix(int(vSkin.x & 0x000000FF));
	const mat4 b1 = fetchMatrix(int(vSkin.x & 0x0000FF00) >> 8);
	const mat4 b2 = fetchMatrix(int(vSkin.x & 0x00FF0000) >> 16);
	const mat4 b3 = fetchMatrix(int(vSkin.x & 0xFF000000) >> 24);
	const float w0 = (vSkin.y & 0x000000FF) / 255.f;
	const float w1 = ((vSkin.y & 0x0000FF00) >> 8) / 255.f;
	const float w2 = ((vSkin.y & 0x00FF0000) >> 16) / 255.f;
	const float w3 = ((vSkin.y & 0xFF000000) >> 24) / 255.f;
	
	vec4 position = vec4(vPosition, 1.f);
	position = b0 * position * w0 + b1 * position * w1 + b2 * position * w2 + b3 * position * w3;
	position.w = 1.f;

	gl_Position = VP * vInstance * position;
	UV = vUV;

	mat3 Matrix = mat3(vInstance);
	vec3 T = normalize(Matrix * vec3(vTangent));
	vec3 N = normalize(Matrix * vNormal);
	vec3 B = cross(N, T);
	mat3 TBN = transpose(mat3(T, B, N));

	vec3 light_direction = normalize(vec3(-0.3f, -0.3f, 0.25f));
	TangentLightDirection = TBN * light_direction;
	
	vertexColor = vec4(geoset_color, layer_alpha);
	if(vertexColor.a <= 0.75f) {
		gl_Position = vec4(0.f);
	}
}