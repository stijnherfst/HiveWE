#include "stdafx.h"

SkinnedMesh::SkinnedMesh(const fs::path& path) {
	shader = resource_manager.load<Shader>({ "Data/Shaders/skinned_mesh_instanced.vs", "Data/Shaders/skinned_mesh_instanced.fs" });

	if (path.extension() == ".mdx" || path.extension() == ".MDX") {
		BinaryReader reader = hierarchy.open_file(path);
		this->path = path;

		size_t vertices = 0;
		size_t indices = 0;
		size_t uvs = 0;
		mdx::MDX model = mdx::MDX(reader);

		has_mesh = model.has_chunk<mdx::GEOS>();
		if (has_mesh) {
			// Calculate required space
			for (auto&& i : model.chunk<mdx::GEOS>()->geosets) {
				vertices += i.vertices.size();
				indices += i.faces.size();
				//uvs += i.texture_coordinate_sets.size() * i.texture_coordinate_sets.front().coordinates.size();
			}

			// Allocate space
			gl->glCreateBuffers(1, &vertex_buffer);
			gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &uv_buffer);
			gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &index_buffer);
			gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

			// Buffer Data
			int base_vertex = 0;
			int base_index = 0;
			for (auto&& i : model.chunk<mdx::GEOS>()->geosets) {
				MeshEntry entry;
				entry.vertices = i.vertices.size();
				entry.base_vertex = base_vertex;

				entry.indices = i.faces.size();
				entry.base_index = base_index;

				entry.material_id = i.material_id;

				entries.push_back(entry);

				gl->glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
				gl->glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().coordinates.data());
				gl->glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

				base_vertex += entry.vertices;
				base_index += entry.indices;
			}
		}

		if (model.has_chunk<mdx::SEQS>()) {
			for (auto&& i : model.chunk<mdx::SEQS>()->sequences) {
				Animation animation;
				animation.interval_start = i.interval_start;
				animation.interval_end = i.interval_end;
				animation.movespeed = i.movespeed;
				animation.flags = i.flags;
				animation.rarity = i.rarity;
				animation.sync_point = i.sync_point;
				animation.extent = i.extent;

				std::transform(i.name.begin(), i.name.end(), i.name.begin(), ::tolower);

				animations.emplace(i.name, animation);
			}
		}

		if (model.has_chunk<mdx::GEOA>()) {
			if (animations.contains("stand")) {
				for (auto&& i : model.chunk<mdx::GEOA>()->animations) {
					if (i.animated_data.has_track(mdx::TrackTag::KGAO)) {
						for (auto&& j : i.animated_data.track<float>(mdx::TrackTag::KGAO)->tracks) {
							if (j.frame >= animations["stand"].interval_start && j.frame <= animations["stand"].interval_end) {
								entries[i.geoset_id].visible = j.value > 0.75;
							}
						}
					}
				}
			}
		}

		if (model.has_chunk<mdx::TEXS>()) {
			for (auto&& i : model.chunk<mdx::TEXS>()->textures) {
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
		}

		mtls = model.chunk<mdx::MTLS>();

		if (model.has_chunk<mdx::BONE>()) {
			for (auto&& i : model.chunk<mdx::BONE>()->bones) {
				//id_to_bone.emplace(i.)
			}
		}
	}
}

SkinnedMesh::~SkinnedMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
}

void SkinnedMesh::render() {
	if (!has_mesh) {
		return;
	}

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	shader->use();

	gl->glUniformMatrix4fv(4, 1, false, &camera->projection_view[0][0]);

	for (auto&& i : entries) {
		if (!i.visible) {
			continue;
		}
		for (auto&& j : mtls->materials[i.material_id].layers) {
			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			gl->glEnable(GL_BLEND);
			gl->glUniform1f(3, -1.f);
			switch (j.blend_mode) {
			case 0:
				gl->glDisable(GL_BLEND);
				break;
			case 1:
				gl->glDisable(GL_BLEND);
				gl->glUniform1f(3, 0.75f); // Alpha test
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

			gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
		}
	}
	gl->glDisableVertexAttribArray(2);

	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribDivisor(2 + i, 0); // ToDo use multiple vao
	}

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glEnable(GL_BLEND);
}