// Shared by the skinned mesh shaders. Mirrors SkinnedFrameData in src/base/render_manager.ixx
// and the layer tables in src/resources/skinned_mesh.ixx.

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

struct DrawInfo {
	uint instance_offset;
	uint bone_offset;
	uint bone_count;
	uint layer_color_offset;
	uint layer_skip_count;
	uint layer_index_global;
	uint layer_index_local;
	uint _pad;
};

/// Bindless texture slots
struct LayerTextureIds {
	uint albedo;
	uint normal;
	uint orm;
	uint emissive;
	uint team_color;
	uint environment;
	uint _pad0;
	uint _pad1;
};

struct LayerParams {
	float alpha_test;
	uint layer_lit;
	uint is_team_color;
	uint _pad;
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Vec4Buffer {
	vec4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer UintBuffer {
	uint values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Uvec2Buffer {
	uvec2 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Uvec4Buffer {
	uvec4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer Mat4Buffer {
	mat4 values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer DrawInfoBuffer {
	DrawInfo values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer LayerTexturesBuffer {
	LayerTextureIds values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer LayerParamsBuffer {
	LayerParams values[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer SkinnedFrame {
	mat4 VP;
	vec4 light_direction;
	uint render_lighting;
	uint _pad0;
	uint _pad1;
	uint _pad2;
	Vec4Buffer layer_colors;
	UintBuffer uvs;
	Uvec2Buffer vertices;
	Vec4Buffer tangents;
	UintBuffer normals;
	Mat4Buffer instance_matrices;
	Uvec4Buffer skins;
	Mat4Buffer bone_matrices;
	UintBuffer team_color_indexes;
	DrawInfoBuffer draw_infos;
	LayerTexturesBuffer layer_textures;
	LayerParamsBuffer layer_params;
};

layout(push_constant, std430) uniform PushConstants {
	SkinnedFrame frame;
	/// Index of the first DrawInfo of this multi-draw
	uint draw_info_base;
} pc;

layout(set = 0, binding = 0) uniform sampler2D textures[];

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

/// Needs to match the extent that it was packed with
vec3 unpack_uvec2_to_vec3(const uvec2 v, const float extent) {
	const uint x = v.y >> 11;
	const uint y = ((v.y & 0x7FFu) << 10) + (v.x >> 22);
	const uint z = v.x & 0x3FFFFFu;

	const float xf = (float(x) / float(1u << 21) * 2.f - 1.f) * extent;
	const float yf = (float(y) / float(1u << 21) * 2.f - 1.f) * extent;
	const float zf = (float(z) / float(1u << 22) * 2.f - 1.f) * extent;

	return vec3(xf, yf, zf);
}

mat4 skin_matrix(SkinnedFrame frame, DrawInfo info, uint instance_id, uvec4 skin) {
	const uint base = info.bone_offset + instance_id * info.bone_count;
	const mat4 b0 = frame.bone_matrices.values[base + (skin.x & 0x0000FFFFu)];
	const mat4 b1 = frame.bone_matrices.values[base + (skin.x >> 16)];
	const mat4 b2 = frame.bone_matrices.values[base + (skin.y & 0x0000FFFFu)];
	const mat4 b3 = frame.bone_matrices.values[base + (skin.y >> 16)];
	const float w0 = (skin.z & 0x0000FFFFu) / 255.f;
	const float w1 = (skin.z >> 16) / 255.f;
	const float w2 = (skin.w & 0x0000FFFFu) / 255.f;
	const float w3 = (skin.w >> 16) / 255.f;
	return b0 * w0 + b1 * w1 + b2 * w2 + b3 * w3;
}
