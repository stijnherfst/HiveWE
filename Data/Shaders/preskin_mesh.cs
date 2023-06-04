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

layout(std430, binding = 3) restrict readonly buffer layoutName3 {
    vec4 positions[];
};

layout(std430, binding = 4) restrict readonly buffer layoutName4 {
    vec4 normals[];
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
    vec4 output_tangent_light_directions[];
};

layout(std430, binding = 9) restrict readonly buffer layoutName9 {
    uvec2 vertices_snorm[];
};

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
	
	// output_vertices[output_index] = VP * skinMatrix * positions[input_index];

	vec2 xy = unpackSnorm2x16(vertices_snorm[input_index].x) * 1024.f;
	vec2 zw = unpackSnorm2x16(vertices_snorm[input_index].y) * 1024.f;

	vec4 vertex = skinMatrix * vec4(xy, zw.x, 1.f);
	output_vertices[output_index] = uvec2(packSnorm2x16(vertex.xy / 1024.f), packSnorm2x16(vertex.zw / 1024.f));

	mat3 model = mat3(instance_matrices[instance_number] * skinMatrix);
	vec3 T = normalize(model * tangents[input_index].xyz);
	vec3 N = normalize(model * vec3(normals[input_index]));
	vec3 B = cross(N, T) * tangents[input_index].w; // to fix handedness
	mat3 TBN = transpose(mat3(T, B, N));

	output_tangent_light_directions[output_index] = vec4(normalize(TBN * light_direction), 1.f);
}