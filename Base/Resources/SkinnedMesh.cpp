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

			for (auto&& vertex_group_id : i.vertex_groups) {
				vertex_groups.push_back(vertex_group_id + matrices);
			}
			matrices += i.matrix_groups.size();
		}

		// Allocate space
		gl->glCreateBuffers(1, &vertex_buffer);
		gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &uv_buffer);
		gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &normal_buffer);
		gl->glNamedBufferData(normal_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &instance_buffer);

		gl->glCreateBuffers(1, &index_buffer);
		gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

		gl->glCreateBuffers(1, &layer_alpha);
		gl->glCreateBuffers(1, &geoset_color);
		gl->glCreateBuffers(1, &retera_node_buffer);
		gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &retera_node_buffer_texture);

		for (const auto& i : model->geosets) {
			int geoset_matrix_offset = 0;
			for (const auto& matrix_size : i.matrix_groups) {
				// show size is capped at 3 because that's what I made the shader do.
				// Not a correct behavior, just a simpler one.
				int show_size = matrix_size > 3 ? 3 : matrix_size;
				glm::uvec4 indices = glm::uvec4(show_size, 0, 0, 0);
				for (int index = 0; index < show_size; index++) {
					indices[index + 1] = i.node_indices[geoset_matrix_offset + index] + 1;
				}
				
				group_indexing_lookups.push_back(indices);
				geoset_matrix_offset += matrix_size;
			}
		}

		gl->glCreateBuffers(1, &retera_groups_buffer);
		gl->glNamedBufferData(retera_groups_buffer, group_indexing_lookups.size() * sizeof(glm::uvec4), group_indexing_lookups.data(), GL_DYNAMIC_DRAW);

		gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &retera_groups_buffer_texture);
		gl->glCreateBuffers(1, &retera_vertex_group_buffer);
		gl->glNamedBufferData(retera_vertex_group_buffer, vertex_groups.size() * sizeof(uint), vertex_groups.data(), GL_STATIC_DRAW);

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

			gl->glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
			gl->glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().data());
			gl->glNamedBufferSubData(normal_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.normals.data());
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

	//for (const auto& i : entries) {
	//	if (!i.visible) {
	//		continue;
	//	}
	//	for (const auto& j : materials[i.material_id].layers) {
	//		if (j.blend_mode == 0 || j.blend_mode == 1) {
	//			continue;
	//		} else {
	//			RenderManager::Inst t;
	//			t.mesh_id = mesh_id;
	//			t.instance_id = render_jobs.size() - 1;
	//			t.distance = glm::distance(camera->position - camera->direction * camera->distance, glm::vec3(model[3]));
	//			map->render_manager.transparent_instances.push_back(t);
	//			return;
	//		}

	//		break; // Currently only draws the first layer
	//	}
	//}
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

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

	// Since a mat4 is 4 vec4's
	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(3 + i);
		gl->glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(3 + i, 1);
	}

	gl->glEnableVertexAttribArray(7);
	gl->glBindBuffer(GL_ARRAY_BUFFER, retera_vertex_group_buffer);
	gl->glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT, 0, nullptr);
	gl->glVertexAttribDivisor(7, 0);

	gl->glTextureBuffer(retera_groups_buffer_texture, GL_RGBA32UI, retera_groups_buffer);
	gl->glBindTextureUnit(1, retera_groups_buffer_texture);

	for (int i = 0; i < render_jobs.size(); i++) {
		instance_bone_matrices.push_back(glm::mat4(0.0f)); // Our shader code currently wants loading "matrix 0" (per instance) to load a totally empty one
		for (int j = 0; j < model->bones.size(); j++) {
			instance_bone_matrices.push_back(skeletons[i]->renderNodes[j].worldMatrix);
		}
	}

	gl->glNamedBufferData(retera_node_buffer, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);
	gl->glTextureBuffer(retera_node_buffer_texture, GL_RGBA32UI, retera_node_buffer);
	gl->glBindTextureUnit(2, retera_node_buffer_texture);

	gl->glUniform1i(10, model->bones.size() + 1);

	for (auto& i : entries) {
		if (!i.visible) {
			continue;
		}
		i.geoset_anim_alphas.clear();
		i.geoset_anim_colors.clear();
		for (int k = 0; k < render_jobs.size(); k++) {
			glm::vec3 geoset_color(1.0f);
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeletons[k]->sequence_index >= 0) {
				geoset_color = i.geoset_anim->getColor(skeletons[k]->current_frame, *skeletons[k]);
				geoset_anim_visibility = i.geoset_anim->getVisibility(skeletons[k]->current_frame, *skeletons[k]);
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
				break;
			}

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			i.layer_alphas.clear();
			for (int k = 0; k < render_jobs.size(); k++) {
				float layer_visibility = 1.0f;
				if (skeletons[k]->sequence_index >= 0) {
					layer_visibility = j.getVisibility(skeletons[k]->current_frame, *skeletons[k]);
				}
				i.layer_alphas.push_back(layer_visibility * i.geoset_anim_alphas[k]);
			}

			gl->glEnableVertexAttribArray(11);
			gl->glBindBuffer(GL_ARRAY_BUFFER, layer_alpha);
			gl->glNamedBufferData(layer_alpha, i.layer_alphas.size() * sizeof(float), i.layer_alphas.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(11, 1);

			gl->glEnableVertexAttribArray(12);
			gl->glBindBuffer(GL_ARRAY_BUFFER, geoset_color);
			gl->glNamedBufferData(geoset_color, i.geoset_anim_colors.size() * sizeof(glm::vec3), i.geoset_anim_colors.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(12, 1);

			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			//break; // Currently only draws the first layer
		}
	}

	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribDivisor(3 + i, 0); // ToDo use multiple vao
	}
}

void SkinnedMesh::render_transparent(int instance_id) const {
	//if (!has_mesh) {
	//	return;
	//}

	//gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	//gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	//gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	//glm::mat4 model = render_jobs[instance_id];
	//model = camera->projection_view * model;

	//gl->glUniformMatrix4fv(0, 1, false, &model[0][0]);

	//for (const auto& i : entries) {
	//	if (!i.visible) {
	//		continue;
	//	}
	//	for (const auto& j : materials[i.material_id].layers) {
	//		if (j.blend_mode == 0 || j.blend_mode == 1) {
	//			continue;
	//		}

	//		switch (j.blend_mode) {
	//		case 2:
	//			gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//			break;
	//		case 3:
	//			gl->glBlendFunc(GL_ONE, GL_ONE);
	//			break;
	//		case 4:
	//			gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//			break;
	//		case 5:
	//			gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	//			break;
	//		case 6:
	//			gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
	//			break;
	//		}

	//		//bool unshaded = j.shading_flags & 0x1;
	//		//bool environment_map = j.shading_flags & 0x2;
	//		//bool unknown1 = j.shading_flags & 0x4;
	//		//bool unknown2 = j.shading_flags & 0x8;
	//		//bool two_sided = j.shading_flags & 0x10;
	//		//bool unfogged = j.shading_flags & 0x20;
	//		//bool no_depth_test = j.shading_flags & 0x30;
	//		//bool no_depth_set = j.shading_flags & 0x40;

	//		gl->glBindTextureUnit(0, textures[j.texture_id]->id);

	//		gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
	//		break; // Currently only draws the first layer
	//	}
	//	//break;
	//}
}