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
    uvec2 vertices[];
};

layout(std430, binding = 4) buffer layoutName4 {
    vec4 normals[];
};

out vec2 UV;
out vec3 Normal;
out vec4 vertexColor;

void main() {
	const uint vertex_index = gl_InstanceID * instance_vertex_count + gl_VertexID;
	// gl_Position = vertices[vertex_index];

	vec2 xy = unpackSnorm2x16(vertices[vertex_index].x) * 1024.f;
	vec2 zw = unpackSnorm2x16(vertices[vertex_index].y) * 1024.f;

	gl_Position = vec4(xy, zw.x, 1.f);

	UV = uvs[gl_VertexID];
	Normal = normals[vertex_index].xyz;
	vertexColor = layer_colors[gl_InstanceID * layer_skip_count + layer_index];
}