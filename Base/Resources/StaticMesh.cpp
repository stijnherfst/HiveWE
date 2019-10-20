#include "StaticMesh.h"

#include "Hierarchy.h"
#include "Camera.h"

#include "HiveWE.h"

StaticMesh::StaticMesh(const fs::path& path) {
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
				uvs += i.texture_coordinate_sets.size() * i.texture_coordinate_sets.front().coordinates.size();
			}
			
			// Allocate space
			gl->glCreateBuffers(1, &vertex_buffer);
			gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &uv_buffer);
			gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

			gl->glCreateBuffers(1, &instance_buffer);

			gl->glCreateBuffers(1, &index_buffer);
			gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

			// Buffer Data
			int base_vertex = 0;
			int base_index = 0;
			for (const auto& i : model.chunk<mdx::GEOS>()->geosets) {
				MeshEntry entry;
				entry.vertices = static_cast<int>(i.vertices.size());
				entry.base_vertex = base_vertex;

				entry.indices = static_cast<int>(i.faces.size());
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
			for (const auto& i : model.chunk<mdx::SEQS>()->sequences) {
				Animation animation;
				animation.interval_start = i.interval_start;
				animation.interval_end = i.interval_end;
				animation.movespeed = i.movespeed;
				animation.flags = i.flags;
				animation.rarity = i.rarity;
				animation.sync_point = i.sync_point;
				animation.extent = i.extent;

				animations.emplace(i.name, animation);
			}
		}

		//if (model.has_chunk<mdx::GEOA>()) {
		//	//for (const auto& i : animations) {
		//	//	if (i.first.starts_with("stand")) {
		//	//		std::cout << i.first << "\n";
		//	//	}
		//	//}
		//	auto tt = model.chunk<mdx::GEOA>()->animations;
		//	if (animations.contains("stand0")) {

		//		auto tt = model.chunk<mdx::GEOA>()->animations;
		//		for (const auto& i : model.chunk<mdx::GEOA>()->animations) {
		//			if (i.animated_data.has_track(mdx::TrackTag::KGAO)) {
		//				auto ttt = i.animated_data.track<float>(mdx::TrackTag::KGAO)->tracks;
		//				for (const auto& j : i.animated_data.track<float>(mdx::TrackTag::KGAO)->tracks) {
		//					if (j.frame <= animations["stand0"].interval_start || j.frame >= animations["stand0"].interval_end) {
		//						entries[i.geoset_id].visible = false;//j.value > 0.75;
		//						break;
		//					} 
		//					//else {
		//					//	entries[i.geoset_id].visible = false;
		//					//}
		//				}
		//			}
		//		}
		//	}
		//}

		mtls = model.chunk<mdx::MTLS>();

		if (model.has_chunk<mdx::TEXS>()) {
			auto tt = model.chunk<mdx::TEXS>()->textures;
			for (const auto& i : model.chunk<mdx::TEXS>()->textures) {
				if (i.replaceable_id != 0) {
					if (!mdx::replacable_id_to_texture.contains(i.replaceable_id)) {
						std::cout << "Unknown replacable ID found\n";
					}
					textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture[i.replaceable_id]));
				} else {
					
					std::string to = i.file_name.stem().string();

					// Only load diffuse to keep memory usage down
					if (to.ends_with("Normal") 
						|| to.ends_with("ORM")
						|| to.ends_with("EnvironmentMap")
						|| to.ends_with("Black32")
						|| to.ends_with("Emissive")) {

						textures.push_back(resource_manager.load<GPUTexture>("Textures/btntempw.dds"));

						for (auto& j : mtls->materials) {
							for (int k = j.layers.size(); k-- > 0;) {
								if (j.layers[k].texture_id == textures.size() - 1) {
									j.layers.erase(j.layers.begin() + k);
									break;
								}
							}
						}

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

		
	}
}

StaticMesh::~StaticMesh() {
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
}

void StaticMesh::render_queue(const glm::mat4& model){
	render_jobs.push_back(model);

	// Register for opaque drawing
	if (render_jobs.size() == 1) {
		map->render_manager.meshes.push_back(this);
		mid = map->render_manager.meshes.size() - 1;
	}

	// Register for transparent drawing
	// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
	if (!has_mesh) {
		return;
	}

	for (const auto& i : entries) {
		if (!i.visible) {
			continue;
		}
		for (const auto& j : mtls->materials[i.material_id].layers) {
			if (j.blend_mode == 0 || j.blend_mode == 1) {
				continue;
			} else {
				RenderManager::Inst t;
				t.mesh_id = mid;
				t.instance_id = render_jobs.size() - 1;
				t.distance = glm::distance(camera->position - camera->direction * camera->distance, glm::vec3(model[3]));
				map->render_manager.transparent_instances.push_back(t);
				return;
			}

			break; // Currently only draws the first layer
		}
	}
}

// Opaque rendering doesn't have to be sorted and can thus be instanced
void StaticMesh::render_opaque() {
	if (!has_mesh) {
		render_jobs.clear();
		return;
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);

	// Since a mat4 is 4 vec4's
	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(2 + i);
		gl->glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(2 + i, 1);
	}

	for (const auto& i : entries) {
		if (!i.visible) {
			continue;
		}
		for (const auto& j : mtls->materials[i.material_id].layers) {
			if (j.blend_mode == 0) {
				gl->glUniform1f(3, -1.f);
			} else if (j.blend_mode == 1) {
				gl->glUniform1f(3, 0.75f);
			} else {
				break;
			}

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);


			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
			break; // Currently only draws the first layer
		}
		//break;
	}

	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribDivisor(2 + i, 0); // ToDo use multiple vao
	}
}

//void StaticMesh::render_transparent() {
//	if (!has_mesh) {
//		return;
//	}
//
//	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
//	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
//
//	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
//	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
//
//	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
//
//	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_STATIC_DRAW);
//
//	// Since a mat4 is 4 vec4's
//	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
//	for (int i = 0; i < 4; i++) {
//		gl->glEnableVertexAttribArray(2 + i);
//		gl->glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
//		gl->glVertexAttribDivisor(2 + i, 1);
//	}
//
//	for (const auto& i : entries) {
//		if (!i.visible) {
//			continue;
//		}
//		for (const auto& j : mtls->materials[i.material_id].layers) {
//			if (j.blend_mode == 0 || j.blend_mode == 1) {
//				continue;
//			}
//
//			switch (j.blend_mode) {
//				case 2:
//					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//					break;
//				case 3:
//					gl->glBlendFunc(GL_ONE, GL_ONE);
//					break;
//				case 4:
//					gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//					break;
//				case 5:
//					gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
//					break;
//				case 6:
//					gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
//					break;
//			}
//
//			gl->glBindTextureUnit(0, textures[j.texture_id]->id);
//
//
//			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
//			break; // Currently only draws the first layer
//		}
//		//break;
//	}
//
//	for (int i = 0; i < 4; i++) {
//		gl->glVertexAttribDivisor(2 + i, 0); // ToDo use multiple vao
//	}
//}

void StaticMesh::render_transparent2(int instance_id) {
	if (!has_mesh) {
		return;
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	glm::mat4 model = render_jobs[instance_id];
	model = camera->projection_view * model;
	
	gl->glUniformMatrix4fv(4, 1, false, &model[0][0]);

	for (const auto& i : entries) {
		if (!i.visible) {
			continue;
		}
		for (const auto& j : mtls->materials[i.material_id].layers) {
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

			gl->glBindTextureUnit(0, textures[j.texture_id]->id);


			gl->glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
			break; // Currently only draws the first layer
		}
		//break;
	}

	
}