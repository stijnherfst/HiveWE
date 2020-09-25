#pragma once

#include <vector>

#include "StaticMesh.h"
#include "SkinnedMesh.h"

class RenderManager {
public:
	struct Inst {
		int mesh_id;
		int instance_id;
		float distance;
	};

	std::shared_ptr<Shader> instance_static_mesh_shader_sd;
	std::shared_ptr<Shader> instance_static_mesh_shader_hd;
	std::shared_ptr<Shader> static_mesh_shader_sd;
	std::shared_ptr<Shader> static_mesh_shader_hd;

	std::shared_ptr<Shader> instance_skinned_mesh_shader_sd;
	std::shared_ptr<Shader> instance_skinned_mesh_shader_hd;
	std::shared_ptr<Shader> skinned_mesh_shader_sd;
	std::shared_ptr<Shader> skinned_mesh_shader_hd;
	std::shared_ptr<Shader> colored_skinned_shader;

	std::vector<StaticMesh*> meshes;
	std::vector<SkinnedMesh*> animated_meshes;
	std::vector<Inst> transparent_instances;
	std::vector<Inst> skinned_transparent_instances;

	GLuint color_buffer;
	GLuint depth_buffer;
	GLuint color_picking_framebuffer;

	int window_width;
	int window_height;

	RenderManager();
	~RenderManager();

	void render(bool render_lighting);

	void resize_framebuffers(int width, int height);
};