#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 4) in uvec2 vSkin;

layout (location = 0) uniform mat4 VP;
layout (location = 3) uniform int bone_count;
layout (location = 4) uniform int layer_skip_count;
layout (location = 5) uniform int layer_index;

layout(std430, binding = 0) buffer layoutName {
    vec4 layer_colors[];
};

layout(std430, binding = 1) buffer layoutName2 {
    mat4 instance_matrices[];
};

layout(std430, binding = 2) buffer layoutName3 {
    mat4 bone_matrices[];
};

out vec2 UV;
out vec3 Normal;
out vec4 vertexColor;

mat4 fetchMatrix(int bone_index) {
	return bone_matrices[gl_InstanceID * bone_count + bone_index];
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

	gl_Position = VP * instance_matrices[gl_InstanceID] * position;

	UV = vUV;
	Normal = normalize(mat3(instance_matrices[gl_InstanceID]) * vNormal);
	vertexColor = layer_colors[gl_InstanceID * layer_skip_count + layer_index];
}