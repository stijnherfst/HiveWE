#version 450 core

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// Mirrors ParticleVertex in particle_emitter2_renderer.ixx
struct ParticleVertex {
	vec3 position;
	vec2 uv;
	vec4 color;
};

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer ParticleVertexBuffer {
	ParticleVertex values[];
};

// Mirrors ParticlePushConstants in vk_editable_mesh.ixx
layout(push_constant, std430) uniform PushConstants {
	mat4 mvp;
	uint texture_slot;
	int filter_mode;
	ParticleVertexBuffer vertices;
} pc;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec4 v_color;

void main() {
	const ParticleVertex vertex = pc.vertices.values[gl_VertexIndex];
	v_uv = vertex.uv;
	v_color = vertex.color;
	gl_Position = pc.mvp * vec4(vertex.position, 1.0);
}
