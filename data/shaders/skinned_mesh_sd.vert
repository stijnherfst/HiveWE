#version 450 core

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require
#include "skinned_mesh_common.glsl"

layout (location = 0) out vec2 UV;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec4 vertexColor;
layout (location = 3) out vec3 team_color;
layout (location = 4) flat out int layer_index;

void main() {
	SkinnedFrame frame = pc.frame;
	const DrawInfo info = frame.draw_infos.values[pc.draw_info_base + uint(gl_DrawIDARB)];
	const uint instance_id = uint(gl_InstanceIndex);
	const uint instance_idx = info.instance_offset + instance_id;

	const mat4 skin = skin_matrix(frame, info, instance_id, frame.skins.values[gl_VertexIndex]);
	const vec3 vertex = unpack_uvec2_to_vec3(frame.vertices.values[gl_VertexIndex], 8192.f);

	gl_Position = frame.VP * frame.instance_matrices.values[instance_idx] * skin * vec4(vertex, 1.f);

	UV = unpackSnorm2x16(frame.uvs.values[gl_VertexIndex]) * 8.f - 1.f;
	vertexColor = frame.layer_colors.values[info.layer_color_offset + instance_id * info.layer_skip_count + info.layer_index_local];
	team_color = team_colors[frame.team_color_indexes.values[instance_idx]];
	layer_index = int(info.layer_index_global);
	Normal = oct_to_float32x3(unpackSnorm2x16(frame.normals.values[gl_VertexIndex]));
}
