#include "render_manager.h"

#include <print>

import ResourceManager;
import Timer;
import MDX;
import Camera;

#include "units.h"
#include "doodads.h"

RenderManager::RenderManager() {
	instance_skinned_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced_sd.vs", "Data/Shaders/skinned_mesh_instanced_sd.fs" });
	instance_skinned_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced_hd.vs", "Data/Shaders/skinned_mesh_instanced_hd.fs" });
	skinned_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_sd.vs", "Data/Shaders/skinned_mesh_sd.fs" });
	skinned_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_hd.vs", "Data/Shaders/skinned_mesh_hd.fs" });
	preskin_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/preskin_mesh.cs" });
	colored_skinned_shader = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instance_color_coded.vs", "Data/Shaders/skinned_mesh_instance_color_coded.fs" });

	glCreateFramebuffers(1, &color_picking_framebuffer);

	glCreateRenderbuffers(1, &color_buffer);
	glNamedRenderbufferStorage(color_buffer, GL_RGBA8, 800, 600);
	glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buffer);

	glCreateRenderbuffers(1, &depth_buffer);
	glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, 800, 600);
	glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

	if (glCheckNamedFramebufferStatus(color_picking_framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::print("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");
	}
}

RenderManager::~RenderManager() {
	glDeleteRenderbuffers(1, &color_buffer);
	glDeleteRenderbuffers(1, &depth_buffer);
	glDeleteFramebuffers(1, &color_picking_framebuffer);
}

void RenderManager::render_queue(SkinnedMesh& skinned_mesh, const SkeletalModelInstance& skeleton, glm::vec3 color) {
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
		RenderManager::SkinnedInstance t{
			.mesh = &skinned_mesh,
			.instance_id = static_cast<int>(skinned_mesh.render_jobs.size() - 1),
			.distance = glm::distance(camera.position - camera.direction * camera.distance, glm::vec3(skeleton.matrix[3]))
		};

		skinned_transparent_instances.push_back(t);
	}
}

void RenderManager::render(bool render_lighting, glm::vec3 light_direction) {
	GLint old_vao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
	
	for (const auto& i : skinned_meshes) {
		i->upload_render_data();
	}

	preskin_mesh_shader->use();
	glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	glUniform3fv(6, 1, &light_direction.x);
	for (const auto& i : skinned_meshes) {
		i->preskin_geometry();
	}
	// Render opaque meshes
	// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
	//instance_skinned_mesh_shader_sd->use();
	//glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	//glUniform1i(2, render_lighting);
	//glUniform3fv(6, 1, &light_direction.x);
	//glBlendFunc(GL_ONE, GL_ZERO);

	//for (const auto& i : skinned_meshes) {
	//	i->render_opaque(false);
	//}

	instance_skinned_mesh_shader_hd->use();
	glUniform1i(2, render_lighting);

	for (const auto& i : skinned_meshes) {
		i->render_opaque(true);
	}

	// Render transparent meshes
	/*std::sort(skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](auto& left, auto& right) { return left.distance > right.distance; });
	glEnable(GL_BLEND);
	glDepthMask(false);

	skinned_mesh_shader_sd->use();
	glUniform1i(2, render_lighting);
	glUniform3fv(8, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, false);
	}

	skinned_mesh_shader_hd->use();
	glUniform1i(2, render_lighting);
	glUniform3fv(8, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, true);
	}*/

	glBindVertexArray(old_vao);

	for (const auto& i : skinned_meshes) {
		i->render_jobs.clear();
		i->render_colors.clear();
		i->skeletons.clear();
		i->instance_bone_matrices.clear();
	}

	glDepthMask(true);

	skinned_meshes.clear();
	skinned_transparent_instances.clear();
}

void RenderManager::resize_framebuffers(int width, int height) {
	glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
	glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
	window_width = width;
	window_height = height;
}

std::optional<size_t> RenderManager::pick_unit_id_under_mouse(Units& units, glm::vec2 mouse_position) {
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

// Requires the OpenGL context to be active/current
std::optional<size_t> RenderManager::pick_doodad_id_under_mouse(Doodads& doodads, glm::vec2 mouse_position) {
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
	for (size_t i = 0; i < doodads.doodads.size(); i++) {
		const Doodad& doodad = doodads.doodads[i];

		mdx::Extent& extent = doodad.mesh->model->sequences[doodad.skeleton.sequence_index].extent;
		if (camera.inside_frustrum(doodad.skeleton.matrix * glm::vec4(extent.minimum, 1.f), doodad.skeleton.matrix * glm::vec4(extent.maximum, 1.f))) {
			doodad.mesh->render_color_coded(doodad.skeleton, i + 1);
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