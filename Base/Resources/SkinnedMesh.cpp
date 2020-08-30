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

	has_mesh = model->geosets.size();
	if (has_mesh) {
		// Calculate required space
		for (auto&& i : model->geosets) {
			if (i.lod != 0) {
				continue;
			}
			vertices += i.vertices.size();
			indices += i.faces.size();
			matrices += i.bone_groups.size();
		}

		// Allocate space
		gl->glCreateBuffers(1, &vertex_buffer);
		gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &uv_buffer);
		gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &normal_buffer);
		gl->glNamedBufferData(normal_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &weight_buffer);
		gl->glNamedBufferData(weight_buffer, vertices * sizeof(glm::uvec2), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &instance_buffer);

		gl->glCreateBuffers(1, &index_buffer);
		gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &layer_alpha);
		gl->glCreateBuffers(1, &geoset_color);
		gl->glCreateBuffers(1, &retera_node_buffer);
		gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &retera_node_buffer_texture);

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
			entry.geoset_anim = nullptr;
			entry.extent = i.extent;

			entries.push_back(entry);

			std::vector<glm::u8vec4> groups;
			std::vector<glm::u8vec4> weights;

			int bone_offset = 0;
			for (const auto& group_size : i.bone_groups) {
				int bone_count = std::min(group_size, 4u);
				glm::uvec4 indices(0);
				glm::uvec4 weightss(0);

				int weight = 255 / bone_count;
				for (int j = 0; j < bone_count; j++) {
					indices[j] = i.bone_indices[bone_offset + j];
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
			if (i.skin.size()) {
				gl->glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * sizeof(glm::vec2), i.skin.data());
			} else {
				gl->glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * sizeof(glm::vec2), skin_weights.data());		
			}
			gl->glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

			base_vertex += entry.vertices;
			base_index += entry.indices;
		}
	}

	// animations geoset ids > geosets
	for (auto& i : model->animations) {
		if (i.geoset_id >= 0 && i.geoset_id < entries.size()) {
			entries[i.geoset_id].geoset_anim = &i;
		}
	}

	materials = model->materials;

	for (const auto& i : model->textures) {
		if (i.replaceable_id != 0) {
			if (!mdx::replacable_id_to_texture.contains(i.replaceable_id)) {
				std::cout << "Unknown replacable ID found\n";
			}
			textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture[i.replaceable_id]));
		} else {

			std::string to = i.file_name.stem().string();

			// Only load diffuse to keep memory usage down
			if (to.ends_with("Normal") || to.ends_with("ORM") || to.ends_with("EnvironmentMap") || to.ends_with("Black32") || to.ends_with("Emissive")) {
				textures.push_back(resource_manager.load<GPUTexture>("Textures/btntempw.dds"));
				continue;
			}

			fs::path new_path = i.file_name;
			new_path.replace_extension(".dds");
			if (hierarchy.file_exists(new_path)) {
				textures.push_back(resource_manager.load<GPUTexture>(new_path));
			} else {
				new_path.replace_extension(".blp");
				if (hierarchy.file_exists(new_path)) {
					textures.push_back(resource_manager.load<GPUTexture>(new_path));
				} else {
					std::cout << "Error loading texture " << i.file_name << "\n";
					textures.push_back(resource_manager.load<GPUTexture>("Textures/btntempw.dds"));
				}
			}

			// ToDo Same texture on different model with different flags?
			gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}
	}
}

SkinnedMesh::~SkinnedMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &normal_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
	// ToDo delete extra buffers
}

void SkinnedMesh::render_queue(SkeletalModelInstance& skeleton) {
	render_jobs.push_back(skeleton.matrix);
	//render_jobs.push_back(model);
	skeletons.push_back(&skeleton);

	// Register for opaque drawing
	if (render_jobs.size() == 1) {
		map->render_manager.animated_meshes.push_back(this);
		mesh_id = map->render_manager.animated_meshes.size() - 1;
	}

	// Register for transparent drawing
	// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
	if (!has_mesh) {
		return;
	}

	for (const auto& i : entries) {
		for (const auto& j : materials[i.material_id].layers) {
			if (j.blend_mode == 0 || j.blend_mode == 1) {
				continue;
			} else {
				RenderManager::Inst t;
				t.mesh_id = mesh_id;
				t.instance_id = render_jobs.size() - 1;
				t.distance = glm::distance(camera->position - camera->direction * camera->distance, glm::vec3(skeleton.matrix[3]));
				map->render_manager.skinned_transparent_instances.push_back(t);
				return;
			}

			break; // Currently only draws the first layer
		}
	}
}

// Opaque rendering doesn't have to be sorted and can thus be instanced
void SkinnedMesh::render_opaque() {
	if (!has_mesh) {
		return;
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, weight_buffer);
	gl->glVertexAttribIPointer(3, 2, GL_UNSIGNED_INT, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

	// Since a mat4 is 4 vec4's
	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(4 + i);
		gl->glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(4 + i, 1);
	}

	for (int i = 0; i < render_jobs.size(); i++) {
		for (int j = 0; j < model->bones.size(); j++) {
			instance_bone_matrices.push_back(skeletons[i]->render_nodes[j].worldMatrix);
		}
	}

	gl->glNamedBufferData(retera_node_buffer, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);
	gl->glTextureBuffer(retera_node_buffer_texture, GL_RGBA32UI, retera_node_buffer);
	gl->glBindTextureUnit(2, retera_node_buffer_texture);

	gl->glUniform1i(3, model->bones.size());

	for (auto& i : entries) {
		i.geoset_anim_alphas.clear();
		i.geoset_anim_colors.clear();
		for (int k = 0; k < render_jobs.size(); k++) {
			glm::vec3 geoset_color(1.0f);
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeletons[k]->sequence_index >= 0) {
				geoset_color = skeletons[k]->get_geoset_animation_color(*i.geoset_anim);
				geoset_anim_visibility = skeletons[k]->get_geoset_animation_visiblity(*i.geoset_anim);
			}
			i.geoset_anim_alphas.push_back(geoset_anim_visibility);
			i.geoset_anim_colors.push_back(geoset_color);
		}
		for (auto& j : materials[i.material_id].layers) {
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

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			i.layer_alphas.clear();
			for (int k = 0; k < render_jobs.size(); k++) {
				float layer_visibility = 1.0f;
				if (skeletons[k]->sequence_index >= 0) {
					layer_visibility = skeletons[k]->get_layer_visiblity(j);
				}
				i.layer_alphas.push_back(layer_visibility * i.geoset_anim_alphas[k]);
			}

			gl->glEnableVertexAttribArray(9);
			gl->glBindBuffer(GL_ARRAY_BUFFER, layer_alpha);
			gl->glNamedBufferData(layer_alpha, i.layer_alphas.size() * sizeof(float), i.layer_alphas.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(9, 1);

			gl->glEnableVertexAttribArray(10);
			gl->glBindBuffer(GL_ARRAY_BUFFER, geoset_color);
			gl->glNamedBufferData(geoset_color, i.geoset_anim_colors.size() * sizeof(glm::vec3), i.geoset_anim_colors.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(10, 1);

			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			break;
		}
	}

	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribDivisor(3 + i, 0); // ToDo use multiple vao
	}
}

void SkinnedMesh::render_transparent(int instance_id) const {
	if (!has_mesh) {
		return;
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, weight_buffer);
	gl->glVertexAttribIPointer(3, 2, GL_UNSIGNED_INT, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glTextureBuffer(retera_node_buffer_texture, GL_RGBA32UI, retera_node_buffer);
	gl->glBindTextureUnit(2, retera_node_buffer_texture);

	glm::mat4 mvp = camera->projection_view * render_jobs[instance_id];
	gl->glUniformMatrix4fv(0, 1, false, &mvp[0][0]);

	gl->glUniform1i(3, model->bones.size() + 1);
	gl->glUniform1i(4, instance_id);

	for (const auto& i : entries) {
		for (const auto& j : materials[i.material_id].layers) {
			if (j.blend_mode == 0 || j.blend_mode == 1) {
				continue;
			}

			switch (j.blend_mode) {
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

			// GhostWolf says always false if blend_mode != 0 || blend_mode != 1
			if (j.shading_flags & 0x80) {
				gl->glDepthMask(false);	
			} else {
				gl->glDepthMask(false);
			}

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
		}
	}
}