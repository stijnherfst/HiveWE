module;

#include <glad/glad.h>

export module RenderManager;

import std;
import types;
import SkinnedMesh;
import SkinnedMeshGlobals;
import Shader;
import SkeletalModelInstance;
import ResourceManager;
import Timer;
import MDX;
import Camera;
import Utilities;
import Globals;
import Units;
import SLK;
import UnorderedMap;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import Doodads;
import Doodad;

export class RenderManager {
	struct PerMeshOffsets {
		uint32_t instance_offset;
		uint32_t bone_offset;
		uint32_t layer_color_offset;
	};

	struct SkinnedInstance {
		SkinnedMesh* mesh;
		uint32_t instance_id;
		float distance;
	};

	std::shared_ptr<Shader> skinned_mesh_shader_sd;
	std::shared_ptr<Shader> skinned_mesh_shader_hd;
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

  public:
	RenderManager() {
		skinned_mesh_globals.init_gl();

		skinned_mesh_shader_sd =
			resource_manager.load<Shader>({"data/shaders/skinned_mesh_sd.vert", "data/shaders/skinned_mesh_sd.frag"}).value();
		skinned_mesh_shader_hd =
			resource_manager.load<Shader>({"data/shaders/skinned_mesh_hd.vert", "data/shaders/skinned_mesh_hd.frag"}).value();
		colored_skinned_shader =
			resource_manager
				.load<Shader>(
					{"data/shaders/skinned_mesh_instance_color_coded.vert", "data/shaders/skinned_mesh_instance_color_coded.frag"}
				)
				.value();

		click_helper = resource_manager.load<SkinnedMesh>("Objects/InvalidObject/InvalidObject.mdx", "", std::nullopt).value();

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

	void
	queue_render(SkinnedMesh& skinned_mesh, const SkeletalModelInstance& skeleton, const glm::vec3 color, const uint32_t team_color_index) {
		const mdx::Extent& extent = skinned_mesh.mdx->sequences[skeleton.sequence_index].extent;

		if (!camera.inside_frustrum_transform(extent.minimum, extent.maximum, skeleton.matrix)) {
			return;
		}

		skinned_mesh.render_jobs.push_back(skeleton.matrix);
		skinned_mesh.render_colors.push_back(color);
		skinned_mesh.render_team_color_indexes.push_back(team_color_index);
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
			skinned_transparent_instances.push_back(
				SkinnedInstance {
					.mesh = &skinned_mesh,
					.instance_id = static_cast<uint32_t>(skinned_mesh.render_jobs.size() - 1),
					.distance = glm::distance(camera.position - camera.direction * camera.distance, glm::vec3(skeleton.matrix[3])),
				}
			);
		}
	}

	// Renders a click helper (little purple checkered box), kinda inefficient but couldn't be bothered writing a whole new rendering path
	void queue_click_helper(const glm::mat4& model) {
		auto a = SkeletalModelInstance(click_helper->mdx);
		a.matrix = model;
		a.update(0.016f);
		click_helper_instances.push_back(a);
	}

	void render(const bool render_lighting, const glm::vec3 light_direction) {
		GLint old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		for (const auto& i : click_helper_instances) {
			queue_render(*click_helper, i, glm::vec3(1.f), 0);
		}

		// Build merged per-frame staging arrays across all meshes.
		hive::unordered_map<const SkinnedMesh*, PerMeshOffsets> mesh_offsets;
		mesh_offsets.reserve(skinned_meshes.size());

		std::vector<glm::mat4> staging_instance;
		std::vector<uint32_t> staging_team_color;
		std::vector<glm::mat4> staging_bones;
		std::vector<glm::vec4> staging_layer_colors;

		for (auto* mesh : skinned_meshes) {
			if (!mesh->has_mesh) {
				continue;
			}
			mesh->make_textures_resident();

			PerMeshOffsets o;
			o.instance_offset = static_cast<uint32_t>(staging_instance.size());
			o.bone_offset = static_cast<uint32_t>(staging_bones.size());
			o.layer_color_offset = static_cast<uint32_t>(staging_layer_colors.size());
			mesh_offsets[mesh] = o;

			staging_instance.insert(staging_instance.end(), mesh->render_jobs.begin(), mesh->render_jobs.end());
			staging_team_color
				.insert(staging_team_color.end(), mesh->render_team_color_indexes.begin(), mesh->render_team_color_indexes.end());

			const size_t bone_count = mesh->mdx->bones.size();
			for (size_t k = 0; k < mesh->render_jobs.size(); k++) {
				staging_bones.insert(
					staging_bones.end(),
					mesh->skeletons[k]->world_matrices.begin(),
					mesh->skeletons[k]->world_matrices.begin() + bone_count
				);

				for (const auto& g : mesh->geosets) {
					glm::vec3 c = mesh->render_colors[k];
					float vis = 1.0f;
					if (g.geoset_anim && mesh->skeletons[k]->sequence_index >= 0) {
						c *= mesh->skeletons[k]->get_geoset_animation_color(*g.geoset_anim);
						vis = mesh->skeletons[k]->get_geoset_animation_visiblity(*g.geoset_anim);
					}
					for (auto& l : mesh->mdx->materials[g.material_id].layers) {
						float lv = mesh->skeletons[k]->sequence_index >= 0 ? mesh->skeletons[k]->get_layer_visiblity(l) : 1.0f;
						staging_layer_colors.emplace_back(c, lv * vis);
					}
				}
			}
		}

		auto& g = skinned_mesh_globals;

		// Single upload per per-frame SSBO.
		if (!staging_instance.empty()) {
			glNamedBufferData(g.instance_ssbo, staging_instance.size() * sizeof(glm::mat4), staging_instance.data(), GL_DYNAMIC_DRAW);
		}
		if (!staging_team_color.empty()) {
			glNamedBufferData(
				g.instance_team_color_index_ssbo,
				staging_team_color.size() * sizeof(uint32_t),
				staging_team_color.data(),
				GL_DYNAMIC_DRAW
			);
		}
		if (!staging_bones.empty()) {
			glNamedBufferData(g.bone_matrices_ssbo, staging_bones.size() * sizeof(glm::mat4), staging_bones.data(), GL_DYNAMIC_DRAW);
		}
		if (!staging_layer_colors.empty()) {
			glNamedBufferData(
				g.layer_colors_ssbo,
				staging_layer_colors.size() * sizeof(glm::vec4),
				staging_layer_colors.data(),
				GL_DYNAMIC_DRAW
			);
		}

		glBindVertexArray(g.vao);
		g.bind_static_ssbos();
		g.bind_per_frame_ssbos();
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g.indirect_buffer);

		// OPAQUE PASS — collapse multi-draw across meshes within each draw-state group.
		skinned_mesh_shader_sd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniform1i(2, render_lighting ? 1 : 0);
		glBlendFunc(GL_ONE, GL_ZERO);
		render_opaque(false, mesh_offsets);

		skinned_mesh_shader_hd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniform1i(2, render_lighting ? 1 : 0);
		render_opaque(true, mesh_offsets);

		// TRANSPARENT PASS — distance-sorted, coalesce adjacent same-state.
		std::ranges::sort(skinned_transparent_instances, [](auto& l, auto& r) {
			return l.distance > r.distance;
		});
		glEnable(GL_BLEND);
		glDepthMask(false);

		skinned_mesh_shader_sd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniform1i(2, render_lighting ? 1 : 0);
		render_transparent(false, mesh_offsets, staging_layer_colors);

		skinned_mesh_shader_hd->use();
		glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniform1i(2, render_lighting ? 1 : 0);
		render_transparent(true, mesh_offsets, staging_layer_colors);

		glDepthMask(true);
		glBindVertexArray(old_vao);

		for (auto* m : skinned_meshes) {
			m->clear_render_data();
		}
		click_helper_instances.clear();
		skinned_meshes.clear();
		skinned_transparent_instances.clear();
	}

	void resize_framebuffers(const int width, const int height) {
		glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
		glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
		window_width = width;
		window_height = height;
	}

	/// Requires the OpenGL context to be active/current.
	/// Returns the unit ID of the unit that is currently under the mouse coordinates.
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	[[nodiscard]]
	std::optional<size_t> pick_unit_id_under_mouse(const Units& units, const glm::vec2 mouse_position) const {
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

			const mdx::Extent& extent = unit.mesh->mdx->sequences[unit.skeleton.sequence_index].extent;
			if (camera.inside_frustrum_transform(extent.minimum, extent.maximum, unit.skeleton.matrix)) {
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
			return {index - 1};
		} else {
			return {};
		}
	}

	/// Requires the OpenGL context to be active/current.
	/// Returns the doodad ID of the doodad that is currently under the mouse coordinates.
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	[[nodiscard]]
	std::optional<size_t> pick_doodad_id_under_mouse(const Doodads& doodads, const glm::vec2 mouse_position) const {
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

		glm::vec3 window = {input_handler.mouse.x, window_height - input_handler.mouse.y, 1.f};
		glm::vec3 pos = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, window_width, window_height));
		glm::vec3 ray_origin = camera.position - camera.direction * camera.distance;
		glm::vec3 ray_direction = glm::normalize(pos - ray_origin);

		colored_skinned_shader->use();
		for (size_t i = 0; i < doodads.doodads.size(); i++) {
			const Doodad& doodad = doodads.doodads[i];

			const mdx::Extent& extent = doodad.mesh->mdx->sequences[doodad.skeleton.sequence_index].extent;
			glm::vec3 local_min = extent.minimum;
			glm::vec3 local_max = extent.maximum;

			const bool is_doodad = doodads_slk.row_headers.contains(doodad.id);
			const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
			bool use_click_helper = slk.data<bool>("useclickhelper", doodad.id);
			if (use_click_helper) {
				local_min = glm::min(local_min, click_helper->mdx->extent.minimum);
				local_max = glm::max(local_max, click_helper->mdx->extent.maximum);
			}

			glm::vec3 min;
			glm::vec3 max;
			// From local space to world space
			transform_aabb_non_uniform(local_min, local_max, min, max, doodad.skeleton.matrix);

			if (intersect_aabb(min, max, ray_origin, ray_direction)) {
				doodad.mesh->render_color_coded(doodad.skeleton, i + 1);

				if (use_click_helper) {
					auto a = SkeletalModelInstance(click_helper->mdx);
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
			return {index - 1};
		} else {
			return {};
		}
	}

  private:
	void render_opaque(const bool render_hd, const hive::unordered_map<const SkinnedMesh*, PerMeshOffsets>& mesh_offsets) const {
		std::vector<SkinnedMeshGlobals::DrawElementsIndirectCommand> commands;
		std::vector<SkinnedMeshGlobals::DrawInfo> draw_infos;

		for (const auto* mesh : skinned_meshes) {
			if (!mesh->has_mesh) {
				continue;
			}
			const auto it = mesh_offsets.find(mesh);
			if (it == mesh_offsets.end()) {
				continue;
			}
			const auto& off = it->second;

			const auto& entries = render_hd ? mesh->opaque_entries_hd : mesh->opaque_entries_sd;
			if (entries.empty()) {
				continue;
			}
			const GLuint instance_count = static_cast<GLuint>(mesh->render_jobs.size());
			const uint32_t bone_count = static_cast<uint32_t>(mesh->mdx->bones.size());
			const uint32_t skip_count = static_cast<uint32_t>(mesh->skip_count);

			for (const auto& e : entries) {
				commands.push_back({
					.count = e.count,
					.instanceCount = instance_count,
					.firstIndex = e.first_index,
					.baseVertex = e.base_vertex,
					.baseInstance = 0,
				});
				SkinnedMeshGlobals::DrawInfo di {};
				di.instance_offset = off.instance_offset;
				di.bone_offset = off.bone_offset;
				di.bone_count = bone_count;
				di.layer_color_offset = off.layer_color_offset;
				di.layer_skip_count = skip_count;
				di.layer_index_global = e.layer_index_global;
				di.layer_index_local = e.layer_index_local;
				draw_infos.push_back(di);
			}
		}

		if (commands.empty()) {
			return;
		}

		// Sort (commands, draw_infos) jointly by DrawState. We don't have DrawState in the command,
		// build a parallel states[] vector while populating.
		std::vector<SkinnedMesh::DrawState> states;
		states.reserve(commands.size());
		{
			size_t idx = 0;
			for (auto* mesh : skinned_meshes) {
				if (!mesh->has_mesh) {
					continue;
				}
				if (mesh_offsets.find(mesh) == mesh_offsets.end()) {
					continue;
				}
				const auto& entries = render_hd ? mesh->opaque_entries_hd : mesh->opaque_entries_sd;
				for (const auto& e : entries) {
					(void)idx;
					states.push_back(e.state);
					idx++;
				}
			}
		}

		std::vector<size_t> perm(commands.size());
		std::iota(perm.begin(), perm.end(), 0u);
		std::ranges::sort(perm, [&](const size_t a, const size_t b) {
			return states[a] < states[b];
		});

		std::vector<SkinnedMeshGlobals::DrawElementsIndirectCommand> sorted_commands(commands.size());
		std::vector<SkinnedMeshGlobals::DrawInfo> sorted_infos(commands.size());
		std::vector<SkinnedMesh::DrawState> sorted_states(commands.size());
		for (size_t i = 0; i < perm.size(); i++) {
			sorted_commands[i] = commands[perm[i]];
			sorted_infos[i] = draw_infos[perm[i]];
			sorted_states[i] = states[perm[i]];
		}

		auto& g = skinned_mesh_globals;
		glNamedBufferData(
			g.indirect_buffer,
			sorted_commands.size() * sizeof(SkinnedMeshGlobals::DrawElementsIndirectCommand),
			sorted_commands.data(),
			GL_DYNAMIC_DRAW
		);
		glNamedBufferData(
			g.draw_infos_ssbo,
			sorted_infos.size() * sizeof(SkinnedMeshGlobals::DrawInfo),
			sorted_infos.data(),
			GL_DYNAMIC_DRAW
		);
		// glNamedBufferData orphans the buffer object, so we must rebind for it to be visible at the binding point.
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, g.draw_infos_ssbo);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g.indirect_buffer);

		size_t groups = 0;

		size_t group_start = 0;
		while (group_start < sorted_commands.size()) {
			size_t group_end = group_start + 1;
			while (group_end < sorted_commands.size() && sorted_states[group_end] == sorted_states[group_start]) {
				group_end++;
			}
			groups += 1;
			const auto& s = sorted_states[group_start];
			glBlendFunc(s.src_factor, s.dst_factor);
			if (s.cull_face) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			if (s.depth_test) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
			glDepthMask(s.depth_mask);

			glUniform1ui(8, static_cast<GLuint>(group_start));
			glMultiDrawElementsIndirect(
				GL_TRIANGLES,
				GL_UNSIGNED_SHORT,
				reinterpret_cast<void*>(group_start * sizeof(SkinnedMeshGlobals::DrawElementsIndirectCommand)),
				static_cast<GLsizei>(group_end - group_start),
				0
			);
			group_start = group_end;
		}
		// std::println("opaque commands {}, groups {}", sorted_commands.size(), groups);
	}

	void render_transparent(
		const bool render_hd,
		const hive::unordered_map<const SkinnedMesh*, PerMeshOffsets>& mesh_offsets,
		const std::vector<glm::vec4>& staging_layer_colors
	) const {
		std::vector<SkinnedMeshGlobals::DrawElementsIndirectCommand> commands;
		std::vector<SkinnedMeshGlobals::DrawInfo> draw_infos;
		std::vector<SkinnedMesh::DrawState> states;

		for (const auto& inst : skinned_transparent_instances) {
			SkinnedMesh* mesh = inst.mesh;
			const auto it = mesh_offsets.find(mesh);
			if (it == mesh_offsets.end()) {
				continue;
			}
			const auto& [instance_offset, bone_offset, layer_color_offset] = it->second;

			const auto& entries = render_hd ? mesh->transparent_entries_hd : mesh->transparent_entries_sd;
			if (entries.empty()) {
				continue;
			}
			const uint32_t bone_count = static_cast<uint32_t>(mesh->mdx->bones.size());
			const uint32_t skip_count = static_cast<uint32_t>(mesh->skip_count);
			const uint32_t instance_id = inst.instance_id;

			for (const auto& e : entries) {
				const size_t color_idx = layer_color_offset + instance_id * skip_count + e.layer_index_local;
				if (color_idx < staging_layer_colors.size() && staging_layer_colors[color_idx].a <= 0.01f) {
					continue;
				}
				commands.push_back({
					.count = e.count,
					.instanceCount = 1u,
					.firstIndex = e.first_index,
					.baseVertex = e.base_vertex,
					.baseInstance = 0,
				});
				SkinnedMeshGlobals::DrawInfo di {};
				di.instance_offset = instance_offset + instance_id;
				di.bone_offset = bone_offset + instance_id * bone_count;
				di.bone_count = bone_count;
				di.layer_color_offset = layer_color_offset + instance_id * skip_count;
				di.layer_skip_count = skip_count;
				di.layer_index_global = e.layer_index_global;
				di.layer_index_local = e.layer_index_local;
				draw_infos.push_back(di);
				states.push_back(e.state);
			}
		}

		if (commands.empty()) {
			return;
		}

		auto& g = skinned_mesh_globals;
		glNamedBufferData(
			g.indirect_buffer,
			commands.size() * sizeof(SkinnedMeshGlobals::DrawElementsIndirectCommand),
			commands.data(),
			GL_DYNAMIC_DRAW
		);
		glNamedBufferData(g.draw_infos_ssbo, draw_infos.size() * sizeof(SkinnedMeshGlobals::DrawInfo), draw_infos.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, g.draw_infos_ssbo);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g.indirect_buffer);

		// Walk in order (distance-sorted) and coalesce only adjacent same-state runs.
		size_t groups = 0;
		size_t group_start = 0;
		while (group_start < commands.size()) {
			size_t group_end = group_start + 1;
			while (group_end < commands.size() && states[group_end] == states[group_start]) {
				group_end++;
			}

			groups += 1;

			const auto& s = states[group_start];
			glBlendFunc(s.src_factor, s.dst_factor);
			if (s.cull_face) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}
			if (s.depth_test) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			glUniform1ui(8, static_cast<GLuint>(group_start));
			glMultiDrawElementsIndirect(
				GL_TRIANGLES,
				GL_UNSIGNED_SHORT,
				reinterpret_cast<void*>(group_start * sizeof(SkinnedMeshGlobals::DrawElementsIndirectCommand)),
				static_cast<GLsizei>(group_end - group_start),
				0
			);
			group_start = group_end;
		}
		// std::println("transparent commands {}, groups {}", commands.size(), groups);
	}
};
