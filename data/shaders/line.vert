#version 450 core

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer Vec3Buffer {
	vec3 values[];
};

// Mirrors LinePushConstants in model_editor_viewport.cpp
layout(push_constant, std430) uniform PushConstants {
	mat4 mvp;
	vec4 color;
	Vec3Buffer positions;
} pc;

void main() {
	gl_Position = pc.mvp * vec4(pc.positions.values[gl_VertexIndex], 1.0);
}
