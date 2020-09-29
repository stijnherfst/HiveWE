#include "RenderManager.h"

#include "ResourceManager.h"
#include "Camera.h"

RenderManager::RenderManager() {
	instance_static_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced_sd.vs", "Data/Shaders/static_mesh_instanced_sd.fs" });
	instance_static_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced_hd.vs", "Data/Shaders/static_mesh_instanced_hd.fs" });
	static_mesh_shader_sd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_sd.vs", "Data/Shaders/static_mesh_sd.fs" });
	static_mesh_shader_hd = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_hd.vs", "Data/Shaders/static_mesh_hd.fs" });
	
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

	if (gl->glCheckNamedFramebufferStatus(color_picking_framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
}

RenderManager::~RenderManager() {
	gl->glDeleteRenderbuffers(1, &color_buffer);
	gl->glDeleteRenderbuffers(1, &depth_buffer);
	gl->glDeleteFramebuffers(1, &color_picking_framebuffer);
}

void RenderManager::render(bool render_lighting) {
	GLint old_vao;
	gl->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	// Render opaque meshes
	// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
	instance_static_mesh_shader_sd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : meshes) {
		i->render_opaque_sd();
	}

	instance_static_mesh_shader_hd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : meshes) {
		i->render_opaque_hd();
	}

	// Skinned
	instance_skinned_mesh_shader_sd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);
		
	for (const auto& i : skinned_meshes) {
		i->render_opaque_sd();
	}

	instance_skinned_mesh_shader_hd->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : skinned_meshes) {
		i->render_opaque_hd();
	}

	//// Render transparent meshes
	std::sort(transparent_instances.begin(), transparent_instances.end(), [](auto& left, auto& right) { return left.distance > right.distance; });
	std::sort(skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](auto& left, auto& right) { return left.distance > right.distance; });

	static_mesh_shader_sd->use();
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : transparent_instances) {
		i.mesh->render_transparent_sd(i.instance_id);
	}

	static_mesh_shader_hd->use();
	gl->glUniform1f(1, -1.f);
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : transparent_instances) {
		i.mesh->render_transparent_hd(i.instance_id);
	}

	// Skinned
	skinned_mesh_shader_sd->use();
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent_sd(i.instance_id);
	}

	skinned_mesh_shader_hd->use();
	gl->glUniform1f(1, -1.f);
	gl->glUniform1i(2, render_lighting);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent_hd(i.instance_id);
	}

	gl->glBindVertexArray(old_vao);

	for (const auto& i : meshes) {
		i->render_jobs.clear();
	}

	for (const auto& i : skinned_meshes) {
		i->render_jobs.clear();
		i->skeletons.clear();
		i->instance_bone_matrices.clear();
	}

	meshes.clear();
	skinned_meshes.clear();
	transparent_instances.clear();
	skinned_transparent_instances.clear();
}

void RenderManager::resize_framebuffers(int width, int height) {
	gl->glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
	gl->glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
	window_width = width;
	window_height = height;
}