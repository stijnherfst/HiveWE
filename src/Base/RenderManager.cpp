#include "RenderManager.h"

import ResourceManager;

#include "Camera.h"
#include "Units.h"
#include "Globals.h"

RenderManager::RenderManager() {
	instance_static_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced_sd.vs", "Data/Shaders/static_mesh_instanced_sd.fs" });
	instance_static_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced_hd.vs", "Data/Shaders/static_mesh_instanced_hd.fs" });
	static_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_sd.vs", "Data/Shaders/static_mesh_sd.fs" });
	static_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_hd.vs", "Data/Shaders/static_mesh_hd.fs" });
	colored_static_shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_color_coded.vs", "Data/Shaders/static_mesh_color_coded.fs" });
	
	instance_skinned_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced_sd.vs", "Data/Shaders/skinned_mesh_instanced_sd.fs" });
	instance_skinned_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced_hd.vs", "Data/Shaders/skinned_mesh_instanced_hd.fs" });
	skinned_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_sd.vs", "Data/Shaders/skinned_mesh_sd.fs" });
	skinned_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_hd.vs", "Data/Shaders/skinned_mesh_hd.fs" });
	colored_skinned_shader = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instance_color_coded.vs", "Data/Shaders/skinned_mesh_instance_color_coded.fs" });

	gl->glCreateFramebuffers(1, &color_picking_framebuffer);

	gl->glCreateRenderbuffers(1, &color_buffer);
	gl->glNamedRenderbufferStorage(color_buffer, GL_RGBA8, 800, 600);
	gl->glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buffer);

	gl->glCreateRenderbuffers(1, &depth_buffer);
	gl->glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, 800, 600);
	gl->glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

	if (gl->glCheckNamedFramebufferStatus(color_picking_framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
}

RenderManager::~RenderManager() {
	gl->glDeleteRenderbuffers(1, &color_buffer);
	gl->glDeleteRenderbuffers(1, &depth_buffer);
	gl->glDeleteFramebuffers(1, &color_picking_framebuffer);
}

void RenderManager::render(bool render_lighting, glm::vec3 light_direction) {
	GLint old_vao;
	gl->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
	
	for (const auto& i : skinned_meshes) {
		i->upload_render_data();
	}

	// Render opaque meshes
	// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
	instance_skinned_mesh_shader_sd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);
	gl->glUniform3fv(6, 1, &light_direction.x);
	gl->glBlendFunc(GL_ONE, GL_ZERO);

	for (const auto& i : skinned_meshes) {
		i->render_opaque(false);
	}

	instance_skinned_mesh_shader_hd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);
	gl->glUniform3fv(6, 1, &light_direction.x);

	for (const auto& i : skinned_meshes) {
		i->render_opaque(true);
	}

	// Render transparent meshes
	std::sort(skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](auto& left, auto& right) { return left.distance > right.distance; });
	gl->glEnable(GL_BLEND);
	gl->glDepthMask(false);

	skinned_mesh_shader_sd->use();
	gl->glUniform1i(2, render_lighting);
	gl->glUniform3fv(8, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, false);
	}

	skinned_mesh_shader_hd->use();
	gl->glUniform1i(2, render_lighting);
	gl->glUniform3fv(8, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, true);
	}

	gl->glBindVertexArray(old_vao);

	for (const auto& i : skinned_meshes) {
		i->render_jobs.clear();
		i->render_colors.clear();
		i->skeletons.clear();
		i->instance_bone_matrices.clear();
	}

	gl->glDepthMask(true);

	skinned_meshes.clear();
	skinned_transparent_instances.clear();
}

void RenderManager::resize_framebuffers(int width, int height) {
	gl->glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
	gl->glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
	window_width = width;
	window_height = height;
}

std::optional<size_t> RenderManager::pick_unit_id_under_mouse(glm::vec2 mouse_position) {
	GLint old_fbo;
	gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
	GLint old_vao;
	gl->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	gl->glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	gl->glViewport(0, 0, window_width, window_height);

	colored_skinned_shader->use();
	for (size_t i = 0; i < map->units.units.size(); i++) {
		const Unit& unit = map->units.units[i];
		if (unit.id == "sloc") {
			continue;
		} // ToDo handle starting locations

		mdx::Extent& extent = unit.mesh->model->sequences[unit.skeleton.sequence_index].extent;
		if (camera->inside_frustrum(unit.matrix * glm::vec4(extent.minimum, 1.f), unit.matrix * glm::vec4(extent.maximum, 1.f))) {
			unit.mesh->render_color_coded(unit.skeleton, i + 1);
		}
	}

	glm::u8vec4 color;
	glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

	gl->glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	gl->glBindVertexArray(old_vao);

	const int index = color.r + (color.g << 8) + (color.b << 16);
	if (index != 0) {
		return { index - 1 };
	} else {
		return {};
	}
}

// Requires the OpenGL context to be active/current
std::optional<size_t> RenderManager::pick_doodad_id_under_mouse(glm::vec2 mouse_position) {
	GLint old_fbo;
	gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
	GLint old_vao;
	gl->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	gl->glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	gl->glViewport(0, 0, window_width, window_height);

	colored_skinned_shader->use();
	for (size_t i = 0; i < map->doodads.doodads.size(); i++) {
		const Doodad& doodad = map->doodads.doodads[i];
		const mdx::Extent& extent = doodad.mesh->model->sequences[doodad.skeleton.sequence_index].extent;
		if (camera->inside_frustrum(doodad.matrix * glm::vec4(extent.minimum, 1.f), doodad.matrix * glm::vec4(extent.maximum, 1.f))) {
			doodad.mesh->render_color_coded(doodad.skeleton, i + 1);
		}
	}

	glm::u8vec4 color;
	glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

	gl->glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	gl->glBindVertexArray(old_vao);

	const int index = color.r + (color.g << 8) + (color.b << 16);
	if (index != 0) {
		return { index - 1 };
	} else {
		return {};
	}
}