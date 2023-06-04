#version 450 core

// Should match the uniform locations in skinned_mesh_sd.vs
layout (location = 4) uniform int instanceID;
layout (location = 6) uniform int layer_skip_count;
layout (location = 7) uniform int layer_index;
layout (location = 9) uniform uint instance_vertex_count;

layout(std430, binding = 0) buffer layoutName {
    vec4 layer_colors[];
};

layout(std430, binding = 1) buffer layoutName1 {
    vec2 uvs[];
};

layout(std430, binding = 2) buffer layoutName2 {
    vec4 vertices[];
};

layout(std430, binding = 3) buffer layoutName3 {
    vec4 tangent_light_directions[];
};

out vec2 UV;
out vec3 tangent_light_direction;
out vec4 vertexColor;

void main() {
	const uint vertex_index = gl_InstanceID * instance_vertex_count + gl_VertexID;
	gl_Position = vertices[vertex_index];

	UV = uvs[gl_VertexID];
	tangent_light_direction = vec3(tangent_light_directions[vertex_index]);
	vertexColor = layer_colors[gl_InstanceID * layer_skip_count + layer_index];
}