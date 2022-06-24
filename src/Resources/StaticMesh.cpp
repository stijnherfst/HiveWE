#include "StaticMesh.h"

import Hierarchy;

#include "Camera.h"

#include "HiveWE.h"

StaticMesh::StaticMesh(const fs::path& path, std::optional<std::pair<int, std::string>> replaceable_id_override) {
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
	gl->glCreateBuffers(1, &color_buffer);

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

	for (size_t i = 0; i < model.textures.size(); i++) {
		const mdx::Texture& texture = model.textures[i];

		if (texture.replaceable_id != 0) {
			// Figure out if this is an HD texture
			// Unfortunately replaceable ID textures don't have any additional information on whether they are diffuse/normal/orm
			// So we take a guess using the index
			std::string suffix;
			bool found = false;
			for (const auto& material : materials) {
				for (const auto& layer : material.layers) {
					for (const auto& texture : layer.textures) {
						if (texture.second.id != i) {
							continue;
						}

						found = true;

						if (layer.hd) {
							switch (texture.first) {
								case 0:
									suffix = "_diffuse";
									break;
								case 1:
									suffix = "_normal";
									break;
								case 2:
									suffix = "_orm";
									break;
								case 3:
									suffix = "_emmisive";
									break;
							}
						}
						break;
					}
					if (found) {
						break;
					}
				}
				if (found) {
					break;
				}
			}

			if (replaceable_id_override && texture.replaceable_id == replaceable_id_override->first) {
				textures.push_back(resource_manager.load<GPUTexture>(replaceable_id_override->second + suffix, std::to_string(texture.flags)));
			} else {
				textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture.at(texture.replaceable_id) + suffix, std::to_string(texture.flags)));
			}
		} else {
			textures.push_back(resource_manager.load<GPUTexture>(texture.file_name, std::to_string(texture.flags)));
		}
		gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, texture.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, texture.flags & 2 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
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

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(4 + i, 1);
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
	gl->glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	gl->glVertexAttribDivisor(8, 1);
}

StaticMesh::~StaticMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &normal_buffer);
	gl->glDeleteBuffers(1, &tangent_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
}

void StaticMesh::render_queue(const glm::mat4& model, glm::vec3 color) {
	// Register for transparent drawing
	// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
	if (!has_mesh) {
		return;
	}

	render_jobs.push_back(model);
	render_colors.push_back(color);

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
	gl->glNamedBufferData(color_buffer, render_colors.size() * sizeof(glm::vec3), render_colors.data(), GL_STATIC_DRAW);

	for (const auto& i : geosets) {
		if (i.hd) {
			continue;
		}

		const auto& layers = materials[i.material_id].layers;
		if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
			continue;
		}

		for (const auto& j : layers) {
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

			//if (j.shading_flags & 0x10) {
			//	gl->glDisable(GL_CULL_FACE);
			//} else {
			//	gl->glEnable(GL_CULL_FACE);
			//}

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

			gl->glBindTextureUnit(0, textures[j.textures.at(0).id]->id);
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

		for (auto& texture : layers[0].textures) {
			gl->glBindTextureUnit(texture.first, textures[texture.second.id]->id);
		}

		gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
	}
}

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

			//if (j.shading_flags & 0x10) {
			//	gl->glDisable(GL_CULL_FACE);
			//} else {
			//	gl->glEnable(GL_CULL_FACE);
			//}

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

			gl->glBindTextureUnit(0, textures[j.textures.at(0).id]->id);

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

		for (auto& texture : layers[0].textures) {
			gl->glBindTextureUnit(texture.first, textures[texture.second.id]->id);
		}

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