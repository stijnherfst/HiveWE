#pragma once

#include <vector>

#include "StaticMesh.h"
#include "SkinnedMesh.h"

class RenderManager {
public:
	struct StaticInstance {
		StaticMesh* mesh;
		int instance_id;
		float distance;
	};

	struct SkinnedInstance {
		SkinnedMesh* mesh;
		int instance_id;
		float distance;
	};

	std::shared_ptr<Shader> instance_static_mesh_shader_sd;
	std::shared_ptr<Shader> instance_static_mesh_shader_hd;
	std::shared_ptr<Shader> static_mesh_shader_sd;
	std::shared_ptr<Shader> static_mesh_shader_hd;
	std::shared_ptr<Shader> colored_static_shader;

	std::shared_ptr<Shader> instance_skinned_mesh_shader_sd;
	std::shared_ptr<Shader> instance_skinned_mesh_shader_hd;
	std::shared_ptr<Shader> skinned_mesh_shader_sd;
	std::shared_ptr<Shader> skinned_mesh_shader_hd;
	std::shared_ptr<Shader> colored_skinned_shader;

	std::vector<StaticMesh*> meshes;
	std::vector<SkinnedMesh*> skinned_meshes;
	std::vector<StaticInstance> transparent_instances;
	std::vector<SkinnedInstance> skinned_transparent_instances;

	GLuint color_buffer;
	GLuint depth_buffer;
	GLuint color_picking_framebuffer;

	int window_width;
	int window_height;

	RenderManager();
	~RenderManager();

	void render(bool render_lighting, glm::vec3 light_direction);

	void resize_framebuffers(int width, int height);
};