module;

#include <vector>
#include <glad/glad.h>
#include <memory>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <print>

#include "units.h"
#include "doodads.h"
#include "globals.h"

export module RenderManager;

import SkinnedMesh;
import Shader;
import SkeletalModelInstance;
import ResourceManager;
import Timer;
import MDX;
import Camera;
import Utilities;

export class RenderManager {
  public:
	struct SkinnedInstance {
		SkinnedMesh* mesh;
		uint32_t instance_id;
		float distance;
	};

	std::shared_ptr<Shader> instance_skinned_mesh_shader_sd;
	std::shared_ptr<Shader> instance_skinned_mesh_shader_hd;
	std::shared_ptr<Shader> skinned_mesh_shader_sd;
	std::shared_ptr<Shader> skinned_mesh_shader_hd;
	std::shared_ptr<Shader> preskin_mesh_shader;
	std::shared_ptr<Shader> colored_skinned_shader;

	std::vector<SkinnedMesh*> skinned_meshes;
	std::vector<SkinnedInstance> skinned_transparent_instances;

	std::shared_ptr<SkinnedMesh> click_helper;
	std::vector<SkeletalModelInstance> click_helper_instances;

	GLuint color_buffer;
	GLuint depth_buffer;
	GLuint color_picking_framebuffer;

	int window_width;
	int window_height;

	RenderManager() {
		instance_skinned_mesh_shader_sd = resource_manager.load<Shader>({ "data/shaders/skinned_mesh_instanced_sd.vert", "data/shaders/skinned_mesh_instanced_sd.frag" });
		instance_skinned_mesh_shader_hd = resource_manager.load<Shader>({ "data/shaders/skinned_mesh_instanced_hd.vert", "data/shaders/skinned_mesh_instanced_hd.frag" });
		skinned_mesh_shader_sd = resource_manager.load<Shader>({ "data/shaders/skinned_mesh_sd.vert", "data/shaders/skinned_mesh_sd.frag" });
		skinned_mesh_shader_hd = resource_manager.load<Shader>({ "data/shaders/skinned_mesh_hd.vert", "data/shaders/skinned_mesh_hd.frag" });
		preskin_mesh_shader = resource_manager.load<Shader>({ "data/shaders/preskin_mesh.comp" });
		colored_skinned_shader = resource_manager.load<Shader>({ "data/shaders/skinned_mesh_instance_color_coded.vert", "data/shaders/skinned_mesh_instance_color_coded.frag" });
		
		click_helper = resource_manager.load<SkinnedMesh>("Objects/InvalidObject/InvalidObject.mdx", "", std::nullopt);

		glCreateFramebuffers(1, &color_picking_framebuffer);

		glCreateRenderbuffers(1, &color_buffer);
		glNamedRenderbufferStorage(color_buffer, GL_RGBA8, 800, 600);
		glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buffer);

		glCreateRenderbuffers(1, &depth_buffer);
		glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, 800, 600);
		glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

		if (glCheckNamedFramebufferStatus(color_picking_framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::println("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");
		}
	}

	~RenderManager() {
		glDeleteRenderbuffers(1, &color_buffer);
		glDeleteRenderbuffers(1, &depth_buffer);
		glDeleteFramebuffers(1, &color_picking_framebuffer);
	}

	void queue_render(SkinnedMesh& skinned_mesh, const SkeletalModelInstance& skeleton, glm::vec3 color) {
		mdx::Extent& extent = skinned_mesh.model->sequences[skeleton.sequence_index].extent;
		if (!camera.inside_frustrum(skeleton.matrix * glm::vec4(extent.minimum, 1.f), skeleton.matrix * glm::vec4(extent.maximum, 1.f))) {
			return;
		}

		skinned_mesh.render_jobs.push_back(skeleton.matrix);
		skinned_mesh.render_colors.push_back(color);
		skinned_mesh.skeletons.push_back(&skeleton);

		// Register for opaque drawing
		if (skinned_mesh.render_jobs.size() == 1) {
			skinned_meshes.push_back(&skinned_mesh);
		}

		// Register for transparent drawing
		// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
		if (!skinned_mesh.has_mesh) {
			return;
		}

		if (skinned_mesh.has_transparent_layers) {
			skinned_transparent_instances.push_back(RenderManager::SkinnedInstance{
				.mesh = &skinned_mesh,
				.instance_id = static_cast<uint32_t>(skinned_mesh.render_jobs.size() - 1),
				.distance = glm::distance(camera.position - camera.direction * camera.distance, glm::vec3(skeleton.matrix[3])),
			});
		}
	}

	// Renders a click helper (little purple checkered box), kinda inefficient but couldn't be bothered writing a whole new rendering path
	void queue_click_helper(glm::mat4 model) {
		auto a = SkeletalModelInstance(click_helper->model);
		a.matrix = model;
		a.update(0.016f);
		click_helper_instances.push_back(a);
	}

	void render(bool render_lighting, glm::vec3 light_direction) {
		GLint old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		for (const auto& i : click_helper_instances) {
			queue_render(*click_helper, i, glm::vec3(1.f));
		}

		uint64_t size = 0;
		for (const auto& i : skinned_meshes) {
			i->upload_render_data(size);
		}

		preskin_mesh_shader->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(6, 1, &light_direction.x);
		for (const auto& i : skinned_meshes) {
			i->preskin_geometry();
		}
		// Render opaque meshes
		// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
		instance_skinned_mesh_shader_sd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glBlendFunc(GL_ONE, GL_ZERO);

		for (const auto& i : skinned_meshes) {
			i->render_opaque(false, render_lighting);
		}

		instance_skinned_mesh_shader_hd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);

		for (const auto& i : skinned_meshes) {
			i->render_opaque(true, render_lighting);
		}

		// Render transparent meshes
		std::sort(skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](auto& left, auto& right) { return left.distance > right.distance; });
		glEnable(GL_BLEND);
		glDepthMask(false);

		skinned_mesh_shader_sd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(8, 1, &light_direction.x);

		for (const auto& i : skinned_transparent_instances) {
			i.mesh->render_transparent(i.instance_id, false, render_lighting);
		}

		skinned_mesh_shader_hd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);

		for (const auto& i : skinned_transparent_instances) {
			i.mesh->render_transparent(i.instance_id, true, render_lighting);
		}

		glBindVertexArray(old_vao);

		for (const auto& i : skinned_meshes) {
			i->render_jobs.clear();
			i->render_colors.clear();
			i->skeletons.clear();
			i->instance_bone_matrices.clear();
		}

		click_helper_instances.clear();

		glDepthMask(true);

		skinned_meshes.clear();
		skinned_transparent_instances.clear();
	}

	void resize_framebuffers(int width, int height) {
		glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
		glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
		window_width = width;
		window_height = height;
	}

	/// Requires the OpenGL context to be active/current
	/// Returns the unit ID of the unit that is currently under the mouse coordinates
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	std::optional<size_t> pick_unit_id_under_mouse(Units& units, glm::vec2 mouse_position) {
		GLint old_fbo;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
		GLint old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

		glClearColor(0, 0, 0, 1);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, window_width, window_height);

		glDepthMask(true);
		glDisable(GL_BLEND);

		colored_skinned_shader->use();
		for (size_t i = 0; i < units.units.size(); i++) {
			const Unit& unit = units.units[i];
			if (unit.id == "sloc") {
				continue;
			} // ToDo handle starting locations

			mdx::Extent& extent = unit.mesh->model->sequences[unit.skeleton.sequence_index].extent;
			if (camera.inside_frustrum(unit.skeleton.matrix * glm::vec4(extent.minimum, 1.f), unit.skeleton.matrix * glm::vec4(extent.maximum, 1.f))) {
				unit.mesh->render_color_coded(unit.skeleton, i + 1);
			}
		}

		glm::u8vec4 color;
		glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

		glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
		glBindVertexArray(old_vao);
		glEnable(GL_BLEND);

		const int index = color.r + (color.g << 8) + (color.b << 16);
		if (index != 0) {
			return { index - 1 };
		} else {
			return {};
		}
	}

	/// Requires the OpenGL context to be active/current
	/// Returns the doodad ID of the doodad that is currently under the mouse coordinates
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	std::optional<size_t> pick_doodad_id_under_mouse(Doodads& doodads, glm::vec2 mouse_position) {
		GLint old_fbo;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
		GLint old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

		glClearColor(0, 0, 0, 1);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, window_width, window_height);

		glDepthMask(true);
		glDisable(GL_BLEND);

		glm::vec3 window = { input_handler.mouse.x, window_height - input_handler.mouse.y, 1.f };
		glm::vec3 pos = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, window_width, window_height));
		glm::vec3 ray_origin = camera.position - camera.direction * camera.distance;
		glm::vec3 ray_direction = glm::normalize(pos - ray_origin);

		colored_skinned_shader->use();
		for (size_t i = 0; i < doodads.doodads.size(); i++) {
			const Doodad& doodad = doodads.doodads[i];

			const mdx::Extent& extent = doodad.mesh->model->sequences[doodad.skeleton.sequence_index].extent;
			glm::vec3 local_min = extent.minimum;
			glm::vec3 local_max = extent.maximum;

			bool is_doodad = doodads_slk.row_headers.contains(doodad.id);
			slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
			bool use_click_helper = slk.data<bool>("useclickhelper", doodad.id);
			if (use_click_helper) {
				local_min = glm::min(local_min, click_helper->model->extent.minimum);
				local_max = glm::max(local_max, click_helper->model->extent.maximum);
			}

			glm::vec3 min;
			glm::vec3 max;
			// From local space to world space
			transform_aabb_non_uniform(local_min, local_max, min, max, doodad.skeleton.matrix);

			if (intersect_aabb(min, max, ray_origin, ray_direction)) {
				doodad.mesh->render_color_coded(doodad.skeleton, i + 1);

				if (use_click_helper) {
					auto a = SkeletalModelInstance(click_helper->model);
					a.matrix = doodad.skeleton.matrix;
					a.update(0.016f);
					click_helper->render_color_coded(a, i + 1);
				}
			}
		}

		glm::u8vec4 color;
		glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

		glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
		glBindVertexArray(old_vao);
		glEnable(GL_BLEND);

		const int index = color.r + (color.g << 8) + (color.b << 16);
		if (index != 0) {
			return { index - 1 };
		} else {
			return {};
		}
	}
};