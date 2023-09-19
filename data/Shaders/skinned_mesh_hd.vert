#version 450 core

// Should match the uniform locations in skinned_mesh_sd.vert
layout (location = 0) uniform mat4 VP;
layout (location = 4) uniform int instanceID;
layout (location = 6) uniform int layer_skip_count;
layout (location = 7) uniform int layer_index;
layout (location = 9) uniform uint instance_vertex_count;

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
    uint tangent_light_directions[];
};

layout(std430, binding = 5) buffer layoutName5 {
    mat4 instance_matrices[];
};

out vec2 UV;
out vec3 tangent_light_direction;
out vec4 vertexColor;

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

void main() {
	const uint vertex_index = instanceID * instance_vertex_count + gl_VertexID;
	vec2 xy = unpackSnorm2x16(vertices[vertex_index].x) * 1024.f;
	vec2 zw = unpackSnorm2x16(vertices[vertex_index].y) * 1024.f;
	gl_Position = VP * instance_matrices[instanceID] * vec4(xy, zw.x, 1.f);

	UV = unpackSnorm2x16(uvs[gl_VertexID]) * 4.f - 1.f;
	tangent_light_direction = oct_to_float32x3(unpackSnorm2x16(tangent_light_directions[vertex_index]));
	vertexColor = layer_colors[instanceID * layer_skip_count + layer_index];
}