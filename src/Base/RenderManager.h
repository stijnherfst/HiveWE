#pragma once

#include <vector>

#include "SkinnedMesh.h"

class RenderManager {
public:
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

	std::vector<SkinnedMesh*> skinned_meshes;
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
	
	/// Returns the unit ID of the unit that is currently under the mouse coordinates
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	std::optional<size_t> pick_unit_id_under_mouse(glm::vec2 mouse_position);

	/// Returns the doodad ID of the doodad that is currently under the mouse coordinates
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	std::optional<size_t> pick_doodad_id_under_mouse(glm::vec2 mouse_position);
};