#version 450 core

layout (location = 0) uniform mat4 MVP;

layout(std430, binding = 0) buffer layoutName {
    mat4 bones[];
};

layout(std430, binding = 1) buffer layoutName1 {
    uvec2 vertices[];
};

layout(std430, binding = 2) buffer layoutName6 {
    uvec2 skins[];
};

vec3 unpack_uvec2_to_vec3(const uvec2 v, const float extent) {
	const uint x = v.y >> 11;
	const uint y = ((v.y & 0x7FFu) << 10) + (v.x >> 22);
	const uint z = v.x & 0x3FFFFFu;

	const float xf = (float(x) / float(1u << 21) * 2.f - 1.f) * extent;
	const float yf = (float(y) / float(1u << 21) * 2.f - 1.f) * extent;
	const float zf = (float(z) / float(1u << 22) * 2.f - 1.f) * extent;

	return vec3(xf, yf, zf);
}

void main() {
	const mat4 b0 = bones[int(skins[gl_VertexID].x & 0x000000FF)];
	const mat4 b1 = bones[int(skins[gl_VertexID].x & 0x0000FF00) >> 8];
	const mat4 b2 = bones[int(skins[gl_VertexID].x & 0x00FF0000) >> 16];
	const mat4 b3 = bones[int(skins[gl_VertexID].x & 0xFF000000) >> 24];
	const float w0 = (skins[gl_VertexID].y & 0x000000FF) / 255.f;
	const float w1 = ((skins[gl_VertexID].y & 0x0000FF00) >> 8) / 255.f;
	const float w2 = ((skins[gl_VertexID].y & 0x00FF0000) >> 16) / 255.f;
	const float w3 = ((skins[gl_VertexID].y & 0xFF000000) >> 24) / 255.f;
	
	vec2 xy = unpackSnorm2x16(vertices[gl_VertexID].x) * 1024.f;
	vec2 zw = unpackSnorm2x16(vertices[gl_VertexID].y) * 1024.f;

	vec4 position = vec4(unpack_uvec2_to_vec3(vertices[gl_VertexID], 4096.f), 1.f);
	position = b0 * position * w0 + b1 * position * w1 + b2 * position * w2 + b3 * position * w3;
	position.w = 1.f;

	gl_Position = MVP * position;

}