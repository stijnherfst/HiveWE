#version 460 core
// #extension GL_EXT_shader_16bit_storage : enable
// GL_EXT_shader_16bit_storage

layout (location = 0) uniform mat4 VP;
layout (location = 1) uniform uint instance_count;
layout (location = 2) uniform uint instance_vertex_count;
layout (location = 3) uniform uint bone_count;
layout (location = 6) uniform vec3 light_direction;

layout(std430, binding = 1) restrict readonly buffer layoutName1 {
    mat4 instance_matrices[];
};

layout(std430, binding = 2) restrict readonly buffer layoutName2 {
    mat4 bone_matrices[];
};

layout(std430, binding = 3) restrict readonly buffer layoutName9 {
    uvec2 vertices_snorm[];
};

layout(std430, binding = 4) restrict readonly buffer layoutName4 {
    uint normals[];
};

layout(std430, binding = 5) restrict readonly buffer layoutName5 {
    vec4 tangents[];
};

layout(std430, binding = 6) restrict readonly buffer layoutName6 {
    uvec2 skins[];
};

layout(std430, binding = 7) restrict writeonly buffer layoutName7 {
    uvec2 output_vertices[];
};

layout(std430, binding = 8) restrict writeonly buffer layoutName8 {
    uint output_tangent_light_directions[];
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

vec2 float32x3_to_oct(in vec3 v) {
	// Project the sphere onto the octahedron, and then onto the xy plane
	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * sign_not_zero(p)) : p;
}

mat4 fetchMatrix(uint instance_number, uint bone_index) {
	return bone_matrices[instance_number * bone_count + bone_index];
}

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
	const uint input_index = gl_GlobalInvocationID.x % instance_vertex_count;
	const uint instance_number = gl_GlobalInvocationID.x / instance_vertex_count;
	const uint output_index = gl_GlobalInvocationID.x;

	const mat4 b0 = fetchMatrix(instance_number, uint(skins[input_index].x & 0x000000FF));
	const mat4 b1 = fetchMatrix(instance_number, uint(skins[input_index].x & 0x0000FF00) >> 8);
	const mat4 b2 = fetchMatrix(instance_number, uint(skins[input_index].x & 0x00FF0000) >> 16);
	const mat4 b3 = fetchMatrix(instance_number, uint(skins[input_index].x & 0xFF000000) >> 24);
	const float w0 = (skins[input_index].y & 0x000000FF) / 255.f;
	const float w1 = ((skins[input_index].y & 0x0000FF00) >> 8) / 255.f;
	const float w2 = ((skins[input_index].y & 0x00FF0000) >> 16) / 255.f;
	const float w3 = ((skins[input_index].y & 0xFF000000) >> 24) / 255.f;
	mat4 skinMatrix = b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3;

	vec2 xy = unpackSnorm2x16(vertices_snorm[input_index].x) * 1024.f;
	vec2 zw = unpackSnorm2x16(vertices_snorm[input_index].y) * 1024.f;

	vec4 vertex = skinMatrix * vec4(xy, zw.x, 1.f);
	output_vertices[output_index] = uvec2(packSnorm2x16(vertex.xy / 1024.f), packSnorm2x16(vertex.zw / 1024.f));

	vec3 normal = oct_to_float32x3(unpackSnorm2x16(normals[input_index]));

	mat3 model = mat3(instance_matrices[instance_number] * skinMatrix);
	vec3 T = normalize(model * tangents[input_index].xyz);
	vec3 N = normalize(model * normal);
	vec3 B = cross(N, T) * tangents[input_index].w; // to fix handedness
	mat3 TBN = transpose(mat3(T, B, N));

	output_tangent_light_directions[output_index] = packSnorm2x16(float32x3_to_oct(normalize(TBN * light_direction)));
}