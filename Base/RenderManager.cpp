#include "RenderManager.h"

#include "ResourceManager.h"
#include "Camera.h"

RenderManager::RenderManager() {
	instance_static_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced.vs", "Data/Shaders/static_mesh_instanced.fs" });
	static_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh.vs", "Data/Shaders/static_mesh.fs" });
	instance_skinned_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced.vs", "Data/Shaders/skinned_mesh_instanced.fs" });
	skinned_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh.vs", "Data/Shaders/skinned_mesh.fs" });
}

void RenderManager::render(bool render_lighting) {
	instance_static_mesh_shader->use();
	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);
	gl->glEnableVertexAttribArray(2);

	// Render opaque meshes
	// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
	gl->glDisable(GL_BLEND);

	for (const auto& i : meshes) {
		i->render_opaque();
	}

	instance_skinned_mesh_shader->use();

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);
	gl->glEnableVertexAttribArray(2);
	gl->glEnableVertexAttribArray(3);

	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniform1i(2, render_lighting);
	for (const auto& i : animated_meshes) {
		i->render_opaque();
	}

	// Render transparent meshes
	std::sort(transparent_instances.begin(), transparent_instances.end(), [](Inst& left, Inst& right) { return left.distance > right.distance; });
	std::sort(skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](Inst& left, Inst& right) { return left.distance > right.distance; });
	static_mesh_shader->use();

	gl->glEnable(GL_BLEND);
	gl->glUniform1f(1, -1.f);
	//gl->glDepthMask(false);
	//gl->glDisable(GL_DEPTH_TEST);
	gl->glDepthMask(true);
	gl->glEnable(GL_DEPTH_TEST);
	for (const auto& i : transparent_instances) {
		meshes[i.mesh_id]->render_transparent(i.instance_id);
	}


	skinned_mesh_shader->use();
	gl->glUniform1f(1, -1.f);
	for (const auto& i : skinned_transparent_instances) {
		animated_meshes[i.mesh_id]->render_transparent(i.instance_id);
	}
	gl->glDepthMask(true);
	gl->glEnable(GL_DEPTH_TEST);

	for (const auto& i : meshes) {
		i->render_jobs.clear();
	}

	for (const auto& i : animated_meshes) {
		i->render_jobs.clear();
		i->skeletons.clear();
		i->instance_bone_matrices.clear();
	}

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glDisableVertexAttribArray(2);
	gl->glDisableVertexAttribArray(3);

	meshes.clear();
	animated_meshes.clear();
	transparent_instances.clear();
	skinned_transparent_instances.clear();
}