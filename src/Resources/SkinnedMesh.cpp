#include "StaticMesh.h"

#include "Hierarchy.h"
#include "Camera.h"

#include "HiveWE.h"

SkinnedMesh::SkinnedMesh(const fs::path& path) {
	if (path.extension() != ".mdx" && path.extension() != ".MDX") {
		throw;
	}

	BinaryReader reader = hierarchy.open_file(path);
	this->path = path;

	size_t vertices = 0;
	size_t indices = 0;
	size_t matrices = 0;

	model = std::make_shared<mdx::MDX>(reader);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	has_mesh = model->geosets.size();
	if (!has_mesh) {
		return;
	}

	// Calculate required space
	for (const auto& i : model->geosets) {
		if (i.lod != 0) {
			continue;
		}
		vertices += i.vertices.size();
		indices += i.faces.size();
		matrices += i.matrix_groups.size();
	}

	// Allocate space
	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &uv_buffer);
	gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &normal_buffer);
	gl->glNamedBufferData(normal_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &tangent_buffer);
	gl->glNamedBufferData(tangent_buffer, vertices * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &weight_buffer);
	gl->glNamedBufferData(weight_buffer, vertices * sizeof(glm::uvec2), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &instance_buffer);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

	gl->glCreateBuffers(1, &bone_matrix_buffer);
	gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &bone_matrix_texture);
	
	gl->glCreateBuffers(1, &layer_colors_ssbo);
	gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, layer_colors_ssbo);

	// Buffer Data
	int base_vertex = 0;
	int base_index = 0;

	for (const auto& i : model->geosets) {
		if (i.lod != 0) {
			continue;
		}
		MeshEntry entry;
		entry.vertices = static_cast<int>(i.vertices.size());
		entry.base_vertex = base_vertex;

		entry.indices = static_cast<int>(i.faces.size());
		entry.base_index = base_index;

		entry.material_id = i.material_id;
		entry.hd = !model->materials[i.material_id].shader_name.empty(); // A heuristic to determine whether a material is SD or HD
		entry.geoset_anim = nullptr;
		entry.extent = i.extent;

		geosets.push_back(entry);

		std::vector<glm::u8vec4> groups;
		std::vector<glm::u8vec4> weights;

		int bone_offset = 0;
		for (const auto& group_size : i.matrix_groups) {
			int bone_count = std::min(group_size, 4u);
			glm::uvec4 indices(0);
			glm::uvec4 weightss(0);

			int weight = 255 / bone_count;
			for (int j = 0; j < bone_count; j++) {
				indices[j] = i.matrix_indices[bone_offset + j];
				weightss[j] = weight;
			}

			int remainder = 255 - weight * bone_count;
			weightss[0] += remainder;

			groups.push_back(indices);
			weights.push_back(weightss);
			bone_offset += group_size;
		}

		std::vector<glm::u8vec4> skin_weights;
		skin_weights.reserve(entry.vertices * 2);
		for (const auto& vertex_group : i.vertex_groups) {
			skin_weights.push_back(groups[vertex_group]);
			skin_weights.push_back(weights[vertex_group]);
		}

		gl->glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
		gl->glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().data());
		gl->glNamedBufferSubData(normal_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.normals.data());
		gl->glNamedBufferSubData(tangent_buffer, base_vertex * sizeof(glm::vec4), entry.vertices * sizeof(glm::vec4), i.tangents.data());
		if (i.skin.size()) {
			gl->glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * sizeof(glm::vec2), i.skin.data());
		} else {
			gl->glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * sizeof(glm::vec2), skin_weights.data());		
		}
		gl->glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

		base_vertex += entry.vertices;
		base_index += entry.indices;
	}

	for (auto& i : geosets) {
		for (auto& j : model->materials[i.material_id].layers) {
			skip_count++;
		}
	}

	// animations geoset ids > geosets
	for (auto& i : model->animations) {
		if (i.geoset_id >= 0 && i.geoset_id < geosets.size()) {
			geosets[i.geoset_id].geoset_anim = &i;
		}
	}

	for (const auto& i : model->textures) {
		if (i.replaceable_id != 0) {
			if (!mdx::replacable_id_to_texture.contains(i.replaceable_id)) {
				std::cout << "Unknown replacable ID found\n";
			}
			textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture[i.replaceable_id]));
		} else {
			textures.push_back(resource_manager.load<GPUTexture>(i.file_name));
			
			// ToDo Same texture on different model with different flags?
			gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}
	}

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);
	gl->glEnableVertexAttribArray(2);
	gl->glEnableVertexAttribArray(3);
	gl->glEnableVertexAttribArray(4);
	gl->glEnableVertexAttribArray(5);
	gl->glEnableVertexAttribArray(6);
	gl->glEnableVertexAttribArray(7);
	gl->glEnableVertexAttribArray(8);

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
	gl->glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, weight_buffer);
	gl->glVertexAttribIPointer(4, 2, GL_UNSIGNED_INT, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(5 + i);
		gl->glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(5 + i, 1);
	}
}

SkinnedMesh::~SkinnedMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &normal_buffer);
	gl->glDeleteBuffers(1, &tangent_buffer);
	gl->glDeleteBuffers(1, &weight_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
	gl->glDeleteBuffers(1, &bone_matrix_buffer);
	gl->glDeleteBuffers(1, &layer_alpha);
	gl->glDeleteBuffers(1, &geoset_color);
}

void SkinnedMesh::render_queue(const SkeletalModelInstance& skeleton, glm::vec3 color) {
	if (model->sequences.size()) {
		mdx::Extent& extent = model->sequences.front().extent; // Todo, use skeleton.sequence_index and test
		if (!camera->inside_frustrum(skeleton.matrix * glm::vec4(extent.minimum, 1.f), skeleton.matrix * glm::vec4(extent.maximum, 1.f))) {
			return;
		}
	}

	render_jobs.push_back(skeleton.matrix);
	render_colors.push_back(color);
	skeletons.push_back(&skeleton);

	// Register for opaque drawing
	if (render_jobs.size() == 1) {
		map->render_manager.skinned_meshes.push_back(this);
	}

	// Register for transparent drawing
	// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
	if (!has_mesh) {
		return;
	}

	for (const auto& i : geosets) {
		const auto& layer = model->materials[i.material_id].layers[0];
		if (layer.blend_mode != 0 && layer.blend_mode != 1) {
			RenderManager::SkinnedInstance t;
			t.mesh = this;
			t.instance_id = render_jobs.size() - 1;
			t.distance = glm::distance(camera->position - camera->direction * camera->distance, glm::vec3(skeleton.matrix[3]));
			map->render_manager.skinned_transparent_instances.push_back(t);
			return;
		}
	}
}

// Opaque rendering doesn't have to be sorted and can thus be instanced
void SkinnedMesh::render_opaque_sd() {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

	for (int i = 0; i < render_jobs.size(); i++) {
		instance_bone_matrices.insert(instance_bone_matrices.end(), skeletons[i]->world_matrices.begin(), skeletons[i]->world_matrices.begin() + model->bones.size());
	}
	gl->glNamedBufferData(bone_matrix_buffer, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);
	gl->glTextureBuffer(bone_matrix_texture, GL_RGBA32UI, bone_matrix_buffer);
	gl->glBindTextureUnit(5, bone_matrix_texture);

	gl->glUniform1i(3, model->bones.size());

	std::vector<glm::vec4> layer_colors;
	for (int k = 0; k < render_jobs.size(); k++) {
		for (auto& i : geosets) {
			glm::vec3 geoset_color(1.f);
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeletons[k]->sequence_index >= 0) {
				geoset_color = skeletons[k]->get_geoset_animation_color(*i.geoset_anim);
				geoset_anim_visibility = skeletons[k]->get_geoset_animation_visiblity(*i.geoset_anim);
			}
			geoset_color *= render_colors[k];

			auto& layers = model->materials[i.material_id].layers;
			for (auto& j : layers) {
				float layer_visibility = 1.0f;
				if (skeletons[k]->sequence_index >= 0) {
					layer_visibility = skeletons[k]->get_layer_visiblity(j);
				}
				layer_colors.push_back(glm::vec4(geoset_color, layer_visibility * geoset_anim_visibility));
			}
		}
	}

	gl->glNamedBufferData(layer_colors_ssbo, layer_colors.size() * sizeof(glm::vec4), layer_colors.data(), GL_STREAM_DRAW);
	gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
	gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, layer_colors_ssbo);


	gl->glUniform1i(4, skip_count);

	int laya = 0;
	for (auto& i : geosets) {
		auto& layers = model->materials[i.material_id].layers;
		if (i.hd) {
			laya += layers.size();
			continue;
		}

		if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
			laya += layers.size();
			continue;
		}

		for (auto& j : layers) {
			gl->glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.f);
			gl->glUniform1i(5, laya);

			switch (j.blend_mode) {
				case 0:
				case 1:
					gl->glBlendFunc(GL_ONE, GL_ZERO);
					break;
				case 2:
					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case 3:
					gl->glBlendFunc(GL_ONE, GL_ONE);
					break;
				case 4:
					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					break;
				case 5:
					gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
					break;
				case 6:
					gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
					break;
			}

			if (j.shading_flags & 0x40) {
				gl->glDisable(GL_DEPTH_TEST);
			} else {
				gl->glEnable(GL_DEPTH_TEST);
			}

			if (j.shading_flags & 0x80) {
				gl->glDepthMask(false);
			} else {
				gl->glDepthMask(true);
			}

			if (j.shading_flags & 0x10) {
				gl->glDisable(GL_CULL_FACE);
			} else {
				gl->glEnable(GL_CULL_FACE);
			}

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			laya++;
		}
	}
}

// Opaque rendering doesn't have to be sorted and can thus be instanced
void SkinnedMesh::render_opaque_hd() {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	gl->glTextureBuffer(bone_matrix_texture, GL_RGBA32UI, bone_matrix_buffer);
	gl->glBindTextureUnit(5, bone_matrix_texture);

	gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
	gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, layer_colors_ssbo);

	gl->glUniform1i(3, model->bones.size());
	gl->glUniform1i(4, skip_count);

	int laya = 0;
	for (auto& i : geosets) {
		if (!i.hd) {
			continue;
		}

		auto& layers = model->materials[i.material_id].layers;
		if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
			continue;
		}

		gl->glUniform1f(1, layers[0].blend_mode == 1 ? 0.75f : -1.f);
		gl->glUniform1i(5, laya);
		laya += layers.size();

		if (layers[0].shading_flags & 0x10) {
			gl->glDisable(GL_CULL_FACE);
		} else {
			gl->glEnable(GL_CULL_FACE);
		}

		if (layers[0].shading_flags & 0x40) {
			gl->glDisable(GL_DEPTH_TEST);
		} else {
			gl->glEnable(GL_DEPTH_TEST);
		}

		if (layers[0].shading_flags & 0x80) {
			gl->glDepthMask(false);
		} else {
			gl->glDepthMask(true);
		}

		for (short i = 0; i < 6; i++)
			gl->glBindTextureUnit(i, textures[layers[i].texture_id]->id);

		gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
	}
}

void SkinnedMesh::render_transparent_sd(int instance_id) {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	glm::mat4 MVP = camera->projection_view * render_jobs[instance_id];
	gl->glUniformMatrix4fv(0, 1, false, &MVP[0][0]);

	gl->glTextureBuffer(bone_matrix_texture, GL_RGBA32UI, bone_matrix_buffer);
	gl->glBindTextureUnit(5, bone_matrix_texture);

	gl->glUniform1i(3, model->bones.size());
	gl->glUniform1i(4, instance_id);
	gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
	gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, layer_colors_ssbo);

	gl->glUniform1i(5, skip_count);

	int laya = 0;
	for (auto& i : geosets) {
		auto& layers = model->materials[i.material_id].layers;
		if (i.hd) {
			laya += layers.size();
			continue;
		}

		if (layers[0].blend_mode == 0 || layers[0].blend_mode == 1) {
			laya += layers.size();
			continue;
		}

		for (auto& j : layers) {
			gl->glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.f);
			gl->glUniform1i(6, laya);

			switch (j.blend_mode) {
				case 0:
				case 1:
					gl->glBlendFunc(GL_ONE, GL_ZERO);
					break;
				case 2:
					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case 3:
					gl->glBlendFunc(GL_ONE, GL_ONE);
					break;
				case 4:
					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					break;
				case 5:
					gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
					break;
				case 6:
					gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
					break;
			}

			if (j.shading_flags & 0x40) {
				gl->glDisable(GL_DEPTH_TEST);
			} else {
				gl->glEnable(GL_DEPTH_TEST);
			}

			if (j.shading_flags & 0x80) {
				gl->glDepthMask(false);
			} else {
				gl->glDepthMask(true);
			}

			if (j.shading_flags & 0x10) {
				gl->glDisable(GL_CULL_FACE);
			} else {
				gl->glEnable(GL_CULL_FACE);
			}

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			laya++;
		}
	}
}

void SkinnedMesh::render_transparent_hd(int instance_id) {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	glm::mat4 MVP = camera->projection_view * render_jobs[instance_id];
	gl->glUniformMatrix4fv(0, 1, false, &MVP[0][0]);
	gl->glUniformMatrix4fv(5, 1, false, &render_jobs[instance_id][0][0]);

	gl->glTextureBuffer(bone_matrix_texture, GL_RGBA32UI, bone_matrix_buffer);
	gl->glBindTextureUnit(5, bone_matrix_texture);

	gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
	gl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, layer_colors_ssbo);

	gl->glUniform1i(3, model->bones.size());
	gl->glUniform1i(4, instance_id);
	gl->glUniform1i(6, skip_count);

	int laya = 0;
	for (auto& i : geosets) {
		auto& layers = model->materials[i.material_id].layers;
		if (!i.hd) {
			laya += layers.size();
			continue;
		}

		if (layers[0].blend_mode == 0 || layers[0].blend_mode == 1) {
			laya += layers.size();
			continue;
		}

		gl->glUniform1i(7, laya);
		laya += layers.size();
		
		switch (layers[0].blend_mode) {
			case 2:
				gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case 3:
				gl->glBlendFunc(GL_ONE, GL_ONE);
				break;
			case 4:
				gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case 5:
				gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;
			case 6:
				gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
				break;
		}

		if (layers[0].shading_flags & 0x40) {
			gl->glDisable(GL_DEPTH_TEST);
		} else {
			gl->glEnable(GL_DEPTH_TEST);
		}

		if (layers[0].shading_flags & 0x80) {
			gl->glDepthMask(false);
		} else {
			gl->glDepthMask(true);
		}

		if (layers[0].shading_flags & 0x10) {
			gl->glDisable(GL_CULL_FACE);
		} else {
			gl->glEnable(GL_CULL_FACE);
		}

		for (short i = 0; i < 6; i++)
			gl->glBindTextureUnit(i, textures[layers[i].texture_id]->id);

		gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
	}
}

void SkinnedMesh::render_color_coded(const SkeletalModelInstance& skeleton, int id) {
	if (!has_mesh) {
		return;
	}

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, weight_buffer);
	gl->glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	glm::mat4 MVP = camera->projection_view * skeleton.matrix;
	gl->glUniformMatrix4fv(0, 1, false, &MVP[0][0]);

	gl->glUniform1i(3, model->bones.size());
	gl->glUniform1i(7, id);

	gl->glUniformMatrix4fv(8, model->bones.size(), false, &instance_bone_matrices[0][0][0]);

	for (auto& i : geosets) {
		glm::vec3 geoset_color(1.0f);
		float geoset_anim_visibility = 1.0f;
		if (i.geoset_anim && skeleton.sequence_index >= 0) {
			geoset_color = skeleton.get_geoset_animation_color(*i.geoset_anim);
			geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*i.geoset_anim);
		}

		for (auto& j : model->materials[i.material_id].layers) {
			if (j.blend_mode == 0) {
				gl->glUniform1f(1, -1.f);
			} else if (j.blend_mode == 1) {
				gl->glUniform1f(1, 0.75f);
			} else {
				continue;
			}

			if (j.shading_flags & 0x40) {
				gl->glDisable(GL_DEPTH_TEST);
			} else {
				gl->glEnable(GL_DEPTH_TEST);
			}

			if (j.shading_flags & 0x80) {
				gl->glDepthMask(false);
			} else {
				gl->glDepthMask(true);
			}

			if (j.shading_flags & 0x10) {
				gl->glDisable(GL_CULL_FACE);
			} else {
				gl->glEnable(GL_CULL_FACE);
			}

			float layer_visibility = 1.0f;
			if (skeleton.sequence_index >= 0) {
				layer_visibility = skeleton.get_layer_visiblity(j);
			}
			float final_visibility = layer_visibility * geoset_anim_visibility;

			gl->glUniform3f(4, geoset_color.x, geoset_color.y, geoset_color.z);
			gl->glUniform1f(5, final_visibility);

			gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
			break;
		}
	}
}