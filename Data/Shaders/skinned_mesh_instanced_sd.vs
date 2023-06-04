#version 450 core

layout (location = 4) uniform int layer_skip_count;
layout (location = 5) uniform int layer_index;
layout (location = 6) uniform uint instance_vertex_count;

layout(std430, binding = 0) buffer layoutName {
    vec4 layer_colors[];
};

layout(std430, binding = 1) buffer layoutName1 {
    vec2 uvs[];
};

layout(std430, binding = 2) buffer layoutName2 {
    vec4 vertices[];
};

layout(std430, binding = 4) buffer layoutName4 {
    vec4 normals[];
};

out vec2 UV;
out vec3 Normal;
out vec4 vertexColor;

void main() {
	const uint vertex_index = gl_InstanceID * instance_vertex_count + gl_VertexID;
	gl_Position = vertices[vertex_index];

	UV = uvs[gl_VertexID];
	Normal = normals[vertex_index].xyz;
	vertexColor = layer_colors[gl_InstanceID * layer_skip_count + layer_index];
}