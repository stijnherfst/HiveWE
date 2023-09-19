#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec4 vTangent;
layout (location = 4) in uvec2 vSkin;

layout (location = 0) uniform mat4 MVP;
layout (location = 3) uniform int bone_count;
layout (location = 4) uniform mat4 model;
layout (location = 5) uniform vec4 layer_color;
layout (location = 6) uniform vec3 light_direction;
layout (location = 8) uniform mat4 bones[217]; // Thats a lotta boners. max 200 because the shader compiler issues an error when going higher

out vec2 UV;
out vec3 tangent_light_direction;
out vec4 vertexColor;

void main() {
	const mat4 b0 = bones[int(vSkin.x & 0x000000FF)];
	const mat4 b1 = bones[int(vSkin.x & 0x0000FF00) >> 8];
	const mat4 b2 = bones[int(vSkin.x & 0x00FF0000) >> 16];
	const mat4 b3 = bones[int(vSkin.x & 0xFF000000) >> 24];
	const float w0 = (vSkin.y & 0x000000FF) / 255.f;
	const float w1 = ((vSkin.y & 0x0000FF00) >> 8) / 255.f;
	const float w2 = ((vSkin.y & 0x00FF0000) >> 16) / 255.f;
	const float w3 = ((vSkin.y & 0xFF000000) >> 24) / 255.f;
	mat4 skinMatrix = model * (b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3);
	
	gl_Position = MVP * skinMatrix * vec4(vPosition, 1.f);
	// gl_Position = MVP * vec4(vPosition, 1.f);

	mat3 model = mat3(skinMatrix);
	vec3 T = normalize(model * vTangent.xyz);
	vec3 N = normalize(model * vNormal);
	vec3 B = cross(N, T) * vTangent.w; // to fix handedness
	mat3 TBN = transpose(mat3(T, B, N));

	UV = vUV;
	tangent_light_direction = normalize(TBN * light_direction);
	vertexColor = layer_color;
}