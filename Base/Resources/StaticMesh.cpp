#include "StaticMesh.h"

#include "Hierarchy.h"
#include "Camera.h"

#include "HiveWE.h"

StaticMesh::StaticMesh(const fs::path& path) {
	if (path.extension() != ".mdx" && path.extension() != ".MDX") {
		throw;
	}

	BinaryReader reader = hierarchy.open_file(path);
	this->path = path;

	size_t vertices = 0;
	size_t indices = 0;
	mdx::MDX model = mdx::MDX(reader);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	has_mesh = model.geosets.size();
	if (!has_mesh) {
		return;
	}

	// Calculate required space
	for (auto&& i : model.geosets) {
		if (i.lod != 0) {
			continue;
		}
		vertices += i.vertices.size();
		indices += i.faces.size();
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

	gl->glCreateBuffers(1, &instance_buffer);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

	// Buffer Data
	int base_vertex = 0;
	int base_index = 0;
	for (const auto& i : model.geosets) {
		if (i.lod != 0) {
			continue;
		}
		MeshEntry entry;
		entry.vertices = static_cast<int>(i.vertices.size());
		entry.base_vertex = base_vertex;

		entry.indices = static_cast<int>(i.faces.size());
		entry.base_index = base_index;

		entry.material_id = i.material_id;
		entry.hd = !model.materials[i.material_id].shader_name.empty(); // A heuristic to determine whether a material is SD or HD
		entry.extent = i.extent;

		geosets.push_back(entry);

		gl->glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
		gl->glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().data());
		gl->glNamedBufferSubData(normal_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.normals.data());
		// ToDo Tangents in MDX format are optional!!
		gl->glNamedBufferSubData(tangent_buffer, base_vertex * sizeof(glm::vec4), entry.vertices * sizeof(glm::vec4), i.tangents.data());
		gl->glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

		base_vertex += entry.vertices;
		base_index += entry.indices;
	}

	materials = model.materials;

	if (model.sequences.size()) {
		extent = model.sequences.front().extent;
	}

	for (const auto& i : model.textures) {
		if (i.replaceable_id != 0) {
			if (!mdx::replacable_id_to_texture.contains(i.replaceable_id)) {
				std::cout << "Unknown replaceable ID found\n";
			}
			textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture[i.replaceable_id]));
		} else {

			std::string to = i.file_name.stem().string();

			// Only load diffuse to keep memory usage down
			//if (to.ends_with("ORM") || to.ends_with("EnvironmentMap") || to.ends_with("Black32") || to.ends_with("Emissive")) {
			//	textures.push_back(resource_manager.load<GPUTexture>("Textures/btntempw.dds"));
			//	continue;
			//}

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

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);
	gl->glEnableVertexAttribArray(2);
	gl->glEnableVertexAttribArray(3);
	gl->glEnableVertexAttribArray(4);
	gl->glEnableVertexAttribArray(5);
	gl->glEnableVertexAttribArray(6);
	gl->glEnableVertexAttribArray(7);

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
	gl->glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(4 + i);
		gl->glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(4 + i, 1);
	}
}

StaticMesh::~StaticMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &normal_buffer);
	gl->glDeleteBuffers(1, &tangent_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
}

void StaticMesh::render_queue(const glm::mat4& model) {
	// Register for transparent drawing
	// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
	if (!has_mesh) {
		return;
	}

	render_jobs.push_back(model);

	// Register for opaque drawing
	if (render_jobs.size() == 1) {
		map->render_manager.meshes.push_back(this);
	}

	for (const auto& i : geosets) {
		const mdx::Layer& layer = materials[i.material_id].layers.front();
		if (layer.blend_mode != 0 && layer.blend_mode != 1) {
			RenderManager::StaticInstance t;
			t.mesh = this;
			t.instance_id = render_jobs.size() - 1;
			t.distance = glm::distance(camera->position - camera->direction * camera->distance, glm::vec3(model[3]));
			map->render_manager.transparent_instances.push_back(t);
			return;
		}
	}
}

void StaticMesh::render_opaque_sd() const {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

	for (const auto& i : geosets) {
		if (i.hd) {
			continue;
		}

		const auto& layers = materials[i.material_id].layers;
		if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
			continue;
		}

		for (const auto j : layers) {
			gl->glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.f);

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

			if (j.shading_flags & 0x10) {
				gl->glDisable(GL_CULL_FACE);
			} else {
				gl->glEnable(GL_CULL_FACE);
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

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);
			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
		}
	}
}

void StaticMesh::render_opaque_hd() const {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	for (const auto& i : geosets) {
		if (!i.hd) {
			continue;
		}

		const auto& layers = materials[i.material_id].layers;
		if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
			continue;
		}
		gl->glUniform1f(1, layers[0].blend_mode == 1 ? 0.75f : -1.f);

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

		gl->glBindTextureUnit(0, textures[layers[0].texture_id]->id); // diffuse
		gl->glBindTextureUnit(1, textures[layers[1].texture_id]->id); // normal
		gl->glBindTextureUnit(2, textures[layers[2].texture_id]->id); // orm

		gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
	}
}

//bool unshaded = j.shading_flags & 0x1;
//bool environment_map = j.shading_flags & 0x2;
//bool unknown1 = j.shading_flags & 0x4;
//bool unknown2 = j.shading_flags & 0x8;
//bool two_sided = j.shading_flags & 0x10;
//bool unfogged = j.shading_flags & 0x20;
//bool no_depth_test = j.shading_flags & 0x30;
//bool no_depth_set = j.shading_flags & 0x40;

void StaticMesh::render_transparent_sd(int instance_id) const {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	glm::mat4 model = render_jobs[instance_id];
	model = camera->projection_view * model;

	gl->glUniformMatrix4fv(0, 1, false, &model[0][0]);

	for (const auto& i : geosets) {
		if (i.hd) {
			continue;
		}

		const auto& layers = materials[i.material_id].layers;
		if (layers[0].blend_mode == 0 || layers[0].blend_mode == 1) {
			continue;
		}
		for (const auto& j : layers) {
			gl->glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.f);

			switch (j.blend_mode) {
				case 0: // Having blend mode None in a geoset with alpha doesn't make sense, but it can happen
				case 1: // ToDo check if blend mode 1 bit alpha Transparent works for alpha geosets
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

			if (j.shading_flags & 0x10) {
				gl->glDisable(GL_CULL_FACE);
			} else {
				gl->glEnable(GL_CULL_FACE);
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

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
		}
	}
}

void StaticMesh::render_transparent_hd(int instance_id) const {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniformMatrix4fv(3, 1, false, &render_jobs[instance_id][0][0]);

	for (const auto& i : geosets) {
		if (!i.hd) {
			continue;
		}

		const auto& layers = materials[i.material_id].layers;
		if (layers[0].blend_mode == 0 || layers[0].blend_mode == 1) {
			continue;
		}

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

		gl->glBindTextureUnit(0, textures[layers[0].texture_id]->id); // diffuse
		gl->glBindTextureUnit(1, textures[layers[1].texture_id]->id); // normal
		gl->glBindTextureUnit(2, textures[layers[2].texture_id]->id); // orm

		gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
	}
}

void StaticMesh::render_color_coded(int id, const glm::mat4& matrix) {
	if (!has_mesh) {
		return;
	}

	gl->glBindVertexArray(vao);

	gl->glUniformMatrix4fv(0, 1, false, &camera->projection_view[0][0]);
	gl->glUniformMatrix4fv(3, 1, false, &matrix[0][0]);
	gl->glUniform1i(7, id);

	for (const auto& i : geosets) {
		const auto& layers = materials[i.material_id].layers;

		switch (layers[0].blend_mode) {
			case 0: // Having blend mode None in a geoset with alpha doesn't make sense, but it can happen
			case 1: // ToDo check if blend mode 1 bit alpha Transparent works for alpha geosets
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

		gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
	}
}