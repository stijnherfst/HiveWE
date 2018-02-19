#include "stdafx.h"

StaticMesh::StaticMesh(const fs::path& path) {
	shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh.vs", "Data/Shaders/static_mesh.fs" });
	shader_instanced = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced.vs", "Data/Shaders/static_mesh_instanced.fs" });

	if (fs::path(path).extension() == ".mdx" || fs::path(path).extension() == ".MDX") {
		BinaryReader reader = hierarchy.open_file(path);

		mdx::MDX model = mdx::MDX(reader);

		if (model.has_chunk<mdx::GEOS>()) {
			has_mesh = true;

			// Calculate required space
			for (auto&& i : model.chunk<mdx::GEOS>()->geosets ) {
				vertices += i.vertices.size();
				indices += i.faces.size();
			}

			// Allocate space
			gl->glCreateBuffers(1, &vertexBuffer);
			gl->glNamedBufferData(vertexBuffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &uvBuffer);
			gl->glNamedBufferData(uvBuffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &instanceBuffer);
			
			gl->glCreateBuffers(1, &indexBuffer);
			gl->glNamedBufferData(indexBuffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

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

				gl->glNamedBufferSubData(vertexBuffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
				gl->glNamedBufferSubData(uvBuffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().coordinates.data());
				gl->glNamedBufferSubData(indexBuffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());
			
				base_vertex += entry.vertices;
				base_index += entry.indices;
			}
		} else {
			has_mesh = false;
		}

		for (auto&& i : model.chunk<mdx::TEXS>()->textures) {
			if (i.file_name == "") {
				if (i.replaceable_id == 1) {
					textures.push_back(resource_manager.load<GPUTexture>("ReplaceableTextures\\TeamColor\\TeamColor00.blp"));
				} else if (i.replaceable_id == 2) {
					textures.push_back(resource_manager.load<GPUTexture>("ReplaceableTextures\\TeamGlow\\TeamGlow00.blp"));
				} else {
					textures.push_back(resource_manager.load<GPUTexture>("ReplaceableTextures\\AshenvaleTree\\AshenTree.blp"));
					textures.back()->id = 0;
				}
			} else {
				textures.push_back(resource_manager.load<GPUTexture>(i.file_name));
				// ToDo Same texture on different model with different flags?
				gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, (i.flags & 1) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
				gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, (i.flags & 1) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			}
		}

		mtls = model.chunk<mdx::MTLS>();
	}
}

StaticMesh::~StaticMesh() {
	gl->glDeleteBuffers(1, &vertexBuffer);
	gl->glDeleteBuffers(1, &uvBuffer);
	gl->glDeleteBuffers(1, &instanceBuffer);
	gl->glDeleteBuffers(1, &indexBuffer);
}

void StaticMesh::render_queue(glm::mat4 mvp){
	render_jobs.push_back(mvp);

	if (render_jobs.size() == 1) {
		map.meshes.push_back(this);
	}
}

void StaticMesh::render() {
	if (!has_mesh) {
		render_jobs.clear();
		return;
	}

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	if (render_jobs.size() < 10) {
		shader->use();

		for (auto&& j : render_jobs) {
			gl->glUniformMatrix4fv(2, 1, GL_FALSE, &j[0][0]);

			for (auto&& i : entries) {
				for (auto&& j : mtls->materials[i.material_id].layers) {
					gl->glBindTextureUnit(0, textures[j.texture_id]->id);

					gl->glEnable(GL_BLEND);
					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ToDo needed?
					gl->glUniform1f(3, -1.f);
					switch (j.blend_mode) {
					case 0:
						gl->glDisable(GL_BLEND);
						break;
					case 1:
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

					gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, (void*)(i.base_index * sizeof(uint16_t)), i.base_vertex);
				}
			}
		}
	} else {
		// ToDo support "Doodads\\Ruins\\Water\\BubbleGeyser\\BubbleGeyser.mdx"

		shader_instanced->use();
		gl->glNamedBufferData(instanceBuffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

		// Since a mat4 is 4 vec4's
		gl->glEnableVertexAttribArray(2);
		gl->glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
		for (int i = 0; i < 4; i++) {
			gl->glEnableVertexAttribArray(2 + i);
			gl->glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const void*)(sizeof(glm::vec4) * i));
			gl->glVertexAttribDivisor(2 + i, 1);
		}

		for (auto&& i : entries) {
			for (auto&& j : mtls->materials[i.material_id].layers) {
				gl->glBindTextureUnit(0, textures[j.texture_id]->id);

				gl->glEnable(GL_BLEND);
				gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ToDo needed?
				gl->glUniform1f(3, -1.f);
				switch (j.blend_mode) {
				case 0:
					gl->glDisable(GL_BLEND);
					break;
				case 1:
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

				gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, (void*)(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			}
		}
		gl->glDisableVertexAttribArray(2);

		for (int i = 0; i < 4; i++) {
			gl->glVertexAttribDivisor(2 + i, 0); // ToDo use multiple vao
		}
	}
	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glEnable(GL_BLEND);

	render_jobs.clear();
}