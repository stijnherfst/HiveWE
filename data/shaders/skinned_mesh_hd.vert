#version 450 core

// Should match the uniform locations in skinned_mesh_sd.vert
layout (location = 0) uniform mat4 VP;
layout (location = 4) uniform int instanceID;
layout (location = 6) uniform int layer_skip_count;
layout (location = 7) uniform int layer_index;
layout (location = 8) uniform vec3 light_direction;
layout (location = 9) uniform uint bone_count;

layout(std430, binding = 0) buffer layoutName {
    vec4 layer_colors[];
};

layout(std430, binding = 1) buffer layoutName1 {
    uint uvs[];
};

layout(std430, binding = 2) buffer layoutName2 {
    uvec2 vertices[];
};

layout(std430, binding = 3) buffer layoutName3 {
	vec4 tangents[];
};

layout(std430, binding = 4) buffer layoutName4 {
	uint normals[];
};

layout(std430, binding = 5) buffer layoutName5 {
    mat4 instance_matrices[];
};

layout(std430, binding = 6) buffer layoutName6 {
	uvec2 skins[];
};

layout(std430, binding = 7) buffer layoutName7 {
	mat4 bone_matrices[];
};

layout(std430, binding = 8) buffer layoutName8 {
	uint team_color_indexes[];
};

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

vec2 sign_not_zero(vec2 v) {
	return vec2((v.x >= 0.f) ? +1.f : -1.f, (v.y >= 0.f) ? +1.f : -1.f);
}

vec3 oct_to_float32x3(vec2 e) {
	vec3 v = vec3(e.xy, 1.f - abs(e.x) - abs(e.y));
	if (v.z < 0.f) {
		v.xy = (1.f - abs(v.yx)) * sign_not_zero(v.xy);
	}
	return normalize(v);
}

/// Needs to match the textent that it was packed with
vec3 unpack_uvec2_to_vec3(const uvec2 v, const float extent) {
	const uint x = v.y >> 11;
	const uint y = ((v.y & 0x7FFu) << 10) + (v.x >> 22);
	const uint z = v.x & 0x3FFFFFu;

	const float xf = (float(x) / float(1u << 21) * 2.f - 1.f) * extent;
	const float yf = (float(y) / float(1u << 21) * 2.f - 1.f) * extent;
	const float zf = (float(z) / float(1u << 22) * 2.f - 1.f) * extent;

	return vec3(xf, yf, zf);
}

mat4 fetchMatrix(uint bone_index) {
	return bone_matrices[instanceID * bone_count + bone_index];
}

void main() {
	const mat4 b0 = fetchMatrix(uint(skins[gl_VertexID].x & 0x000000FF));
	const mat4 b1 = fetchMatrix(uint(skins[gl_VertexID].x & 0x0000FF00) >> 8);
	const mat4 b2 = fetchMatrix(uint(skins[gl_VertexID].x & 0x00FF0000) >> 16);
	const mat4 b3 = fetchMatrix(uint(skins[gl_VertexID].x & 0xFF000000) >> 24);
	const float w0 = (skins[gl_VertexID].y & 0x000000FF) / 255.f;
	const float w1 = ((skins[gl_VertexID].y & 0x0000FF00) >> 8) / 255.f;
	const float w2 = ((skins[gl_VertexID].y & 0x00FF0000) >> 16) / 255.f;
	const float w3 = ((skins[gl_VertexID].y & 0xFF000000) >> 24) / 255.f;
	const mat4 skin_matrix = b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3;

	const vec3 vertex = unpack_uvec2_to_vec3(vertices[gl_VertexID], 8192.f);

	gl_Position = VP * instance_matrices[instanceID] * skin_matrix * vec4(vertex, 1.f);

	vec3 normal = oct_to_float32x3(unpackSnorm2x16(normals[gl_VertexID]));

	mat3 model = mat3(instance_matrices[instanceID] * skin_matrix);
	vec3 T = normalize(model * tangents[gl_VertexID].xyz);
	vec3 N = normalize(model * normal);
	vec3 B = cross(N, T) * tangents[gl_VertexID].w; // to fix handedness
	mat3 TBN = transpose(mat3(T, B, N));

	UV = unpackSnorm2x16(uvs[gl_VertexID]) * 8.f - 1.f;
	tangent_light_direction = normalize(TBN * light_direction);
	vertexColor = layer_colors[instanceID * layer_skip_count + layer_index];
	team_color = team_colors[team_color_indexes[instanceID]];
}