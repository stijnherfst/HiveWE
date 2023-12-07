module;

#include <print>
#include <filesystem>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/packing.hpp>

export module SkinnedMesh;

import MDX;
import ResourceManager;
import GPUTexture;
import Shader;
import Hierarchy;
import Camera;
import SkeletalModelInstance;
import Utilities;

namespace fs = std::filesystem;

export class SkinnedMesh : public Resource {
  public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		mdx::Extent extent;

		mdx::GeosetAnimation* geoset_anim; // can be nullptr, often
	};

	std::shared_ptr<mdx::MDX> model;

	std::vector<MeshEntry> geosets;
	bool has_mesh; // ToDo remove when added support for meshless
	bool has_transparent_layers = false;

	uint32_t instance_vertex_count = 0;

	GLuint vao;
	GLuint vertex_snorm_buffer;
	GLuint uv_snorm_buffer;
	GLuint normal_buffer;
	GLuint tangent_buffer;
	GLuint weight_buffer;
	GLuint index_buffer;
	GLuint layer_alpha;

	GLuint instance_ssbo;
	GLuint layer_colors_ssbo;
	GLuint layer_texture_ssbo;
	GLuint bones_ssbo;
	GLuint bones_ssbo_colored;

	GLuint preskinned_vertex_ssbo;
	GLuint preskinned_tangent_light_direction_ssbo;

	int skip_count = 0;

	fs::path path;
	std::vector<std::shared_ptr<GPUTexture>> textures;
	std::vector<glm::mat4> render_jobs;
	std::vector<glm::vec3> render_colors;
	std::vector<const SkeletalModelInstance*> skeletons;
	std::vector<glm::mat4> instance_bone_matrices;
	std::vector<glm::vec4> layer_colors;

	static constexpr const char* name = "SkinnedMesh";

	explicit SkinnedMesh(const fs::path& path, std::optional<std::pair<int, std::string>> replaceable_id_override) {
		if (path.extension() != ".mdx" && path.extension() != ".MDX") {
			throw;
		}

		BinaryReader reader = hierarchy.open_file(path);
		this->path = path;

		size_t vertices = 0;
		size_t indices = 0;
		size_t matrices = 0;

		model = std::make_shared<mdx::MDX>(reader);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		has_mesh = model->geosets.size();
		if (!has_mesh) {
			return;
		}

		for (const auto& i : model->geosets) {
			const auto& layer = model->materials[i.material_id].layers[0];
			if (layer.blend_mode != 0 && layer.blend_mode != 1) {
				has_transparent_layers = true;
				break;
			}
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
		glCreateBuffers(1, &vertex_snorm_buffer);
		glNamedBufferStorage(vertex_snorm_buffer, vertices * sizeof(glm::uvec2), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &uv_snorm_buffer);
		glNamedBufferStorage(uv_snorm_buffer, vertices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &normal_buffer);
		glNamedBufferStorage(normal_buffer, vertices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &tangent_buffer);
		glNamedBufferStorage(tangent_buffer, vertices * sizeof(glm::vec4), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &weight_buffer);
		glNamedBufferStorage(weight_buffer, vertices * sizeof(glm::uvec2), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &index_buffer);
		glNamedBufferStorage(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);

		glCreateBuffers(1, &instance_ssbo);
		glCreateBuffers(1, &layer_colors_ssbo);
		glCreateBuffers(1, &layer_texture_ssbo);
		glCreateBuffers(1, &bones_ssbo);
		glCreateBuffers(1, &bones_ssbo_colored);

		glCreateBuffers(1, &preskinned_vertex_ssbo);
		glCreateBuffers(1, &preskinned_tangent_light_direction_ssbo);

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

			geosets.push_back(entry);

			// If the skin vector is empty then the model has SD bone weights and we convert them to the HD skin weights.
			// Technically SD supports infinite bones per vertex, but we limit it to 4 like HD does.
			// This could cause graphical inconsistencies with the game, but after more than 4 bones the contribution per bone is low enough that we don't care
			if (i.skin.empty()) {
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

				glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * 8, skin_weights.data());
			} else {
				glNamedBufferSubData(weight_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * 8, i.skin.data());
			}

			std::vector<glm::uvec2> vertices_snorm;
			for (const auto& j : i.vertices) {
				uint32_t xy = glm::packSnorm2x16(glm::vec2(j.x / 1024.f, j.y / 1024.f));
				uint32_t zw = glm::packSnorm2x16(glm::vec2(j.z / 1024.f, 0.0));

				vertices_snorm.push_back(glm::uvec2(xy, zw));
			}
			glNamedBufferSubData(vertex_snorm_buffer, base_vertex * sizeof(glm::uvec2), entry.vertices * sizeof(glm::uvec2), vertices_snorm.data());

			std::vector<uint32_t> uvs_snorm;
			for (const auto& j : i.texture_coordinate_sets.front()) {
				uvs_snorm.push_back(glm::packSnorm2x16((j + 1.f) / 4.f));
			}
			glNamedBufferSubData(uv_snorm_buffer, base_vertex * sizeof(uint32_t), entry.vertices * sizeof(uint32_t), uvs_snorm.data());

			std::vector<uint32_t> normals_oct_snorm;
			for (const auto& normal : i.normals) {
				normals_oct_snorm.push_back(glm::packSnorm2x16(float32x3_to_oct(normal)));
			}
			glNamedBufferSubData(normal_buffer, base_vertex * sizeof(uint32_t), entry.vertices * sizeof(uint32_t), normals_oct_snorm.data());

			if (!i.tangents.empty()) {
				glNamedBufferSubData(tangent_buffer, base_vertex * sizeof(glm::vec4), entry.vertices * sizeof(glm::vec4), i.tangents.data());
			} else {
				//glNamedBufferSubData(tangent_buffer, base_vertex * sizeof(glm::vec4), entry.vertices * sizeof(glm::vec4), normals_vec4.data());
			}

			glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

			base_vertex += entry.vertices;
			base_index += entry.indices;
		}

		for (const auto& i : geosets) {
			skip_count += model->materials[i.material_id].layers.size();
			instance_vertex_count += i.indices;
		}

		// animations geoset ids > geosets
		for (auto& i : model->animations) {
			if (i.geoset_id >= 0 && i.geoset_id < geosets.size()) {
				geosets[i.geoset_id].geoset_anim = &i;
			}
		}

		for (size_t i = 0; i < model->textures.size(); i++) {
			const mdx::Texture& texture = model->textures[i];
			
			if (texture.replaceable_id != 0) {
				// Figure out if this is an HD texture
				// Unfortunately replaceable ID textures don't have any additional information on whether they are diffuse/normal/orm
				// So we take a guess using the index
				std::string suffix("");
				bool found = false;
				for (const auto& material : model->materials) {
					for (const auto& layer : material.layers) {
						for (size_t j = 0; j < layer.texturess.size(); j++) {
							if (layer.texturess[j].id != i) {
								continue;
							}

							found = true;

							if (layer.hd) {
								switch (j) {
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
										suffix = "_emissive";
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
			glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, texture.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, texture.flags & 2 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}

		// Reclaim some space
		for (auto& i : model->geosets) {
			i.vertices.shrink_to_fit();
			i.vertices.clear();
			i.normals.clear();
			i.normals.shrink_to_fit();
			i.face_type_groups.clear();
			i.face_type_groups.shrink_to_fit();
			i.face_groups.clear();
			i.face_groups.shrink_to_fit();
			i.faces.clear();
			i.faces.shrink_to_fit();
			i.vertex_groups.clear();
			i.vertex_groups.shrink_to_fit();
			i.matrix_groups.clear();
			i.matrix_groups.shrink_to_fit();
			i.matrix_indices.clear();
			i.matrix_indices.shrink_to_fit();
			i.extents.clear();
			i.extents.shrink_to_fit();
			i.tangents.clear();
			i.tangents.shrink_to_fit();
			i.skin.clear();
			i.skin.shrink_to_fit();
			i.texture_coordinate_sets.clear();
			i.texture_coordinate_sets.shrink_to_fit();
		}

		glVertexArrayElementBuffer(vao, index_buffer);
	}

	~SkinnedMesh() {
		glDeleteBuffers(1, &vertex_snorm_buffer);
		glDeleteBuffers(1, &uv_snorm_buffer);
		glDeleteBuffers(1, &normal_buffer);
		glDeleteBuffers(1, &tangent_buffer);
		glDeleteBuffers(1, &weight_buffer);
		glDeleteBuffers(1, &index_buffer);
		glDeleteBuffers(1, &layer_alpha);
		glDeleteBuffers(1, &layer_colors_ssbo);
		glDeleteBuffers(1, &layer_texture_ssbo);
		glDeleteBuffers(1, &instance_ssbo);
		glDeleteBuffers(1, &bones_ssbo);
		glDeleteBuffers(1, &bones_ssbo_colored);

		glDeleteBuffers(1, &preskinned_vertex_ssbo);
		glDeleteBuffers(1, &preskinned_tangent_light_direction_ssbo);
	}

	void upload_render_data(uint64_t& size) {
		if (!has_mesh) {
			return;
		}

		glNamedBufferData(instance_ssbo, render_jobs.size() * sizeof(glm::mat4), render_jobs.data(), GL_DYNAMIC_DRAW);

		for (int i = 0; i < render_jobs.size(); i++) {
			instance_bone_matrices.insert(instance_bone_matrices.end(), skeletons[i]->world_matrices.begin(), skeletons[i]->world_matrices.begin() + model->bones.size());
		}

		glNamedBufferData(bones_ssbo, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);

		layer_colors.clear();

		for (size_t k = 0; k < render_jobs.size(); k++) {
			for (const auto& i : geosets) {
				glm::vec3 geoset_color = render_colors[k];
				float geoset_anim_visibility = 1.0f;
				if (i.geoset_anim && skeletons[k]->sequence_index >= 0) {
					geoset_color *= skeletons[k]->get_geoset_animation_color(*i.geoset_anim);
					geoset_anim_visibility = skeletons[k]->get_geoset_animation_visiblity(*i.geoset_anim);
				}

				const auto& layers = model->materials[i.material_id].layers;
				for (auto& j : layers) {
					float layer_visibility = 1.0f;
					if (skeletons[k]->sequence_index >= 0) {
						layer_visibility = skeletons[k]->get_layer_visiblity(j);
					}
					layer_colors.push_back(glm::vec4(geoset_color, layer_visibility * geoset_anim_visibility));
				}
			}
		}

		glNamedBufferData(layer_colors_ssbo, layer_colors.size() * sizeof(glm::vec4), layer_colors.data(), GL_DYNAMIC_DRAW);

		glNamedBufferData(preskinned_vertex_ssbo, instance_vertex_count * sizeof(glm::uvec2) * render_jobs.size(), nullptr, GL_DYNAMIC_DRAW);
		glNamedBufferData(preskinned_tangent_light_direction_ssbo, instance_vertex_count * sizeof(uint32_t) * render_jobs.size(), nullptr, GL_DYNAMIC_DRAW);
	}

	// Render all geometry and save the resulting vertices in a buffer
	void preskin_geometry() {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(vao);

		glUniform1ui(1, render_jobs.size());
		glUniform1ui(2, instance_vertex_count);
		glUniform1ui(3, model->bones.size());

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, instance_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bones_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vertex_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normal_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, tangent_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, weight_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, preskinned_vertex_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, preskinned_tangent_light_direction_ssbo);

		glDispatchCompute(((instance_vertex_count * render_jobs.size()) + 63) / 64, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	void render_opaque(bool render_hd, bool render_lighting) {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(vao);

		glUniform1i(4, skip_count);
		glUniform1ui(6, instance_vertex_count);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uv_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, preskinned_vertex_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, preskinned_tangent_light_direction_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normal_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, instance_ssbo);

		int lay_index = 0;
		for (const auto& i : geosets) {
			const auto& layers = model->materials[i.material_id].layers;

			if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
				lay_index += layers.size();
				continue;
			}

			for (const auto& j : layers) {
				// We don't have to render fully transparent meshes
				// Some Reforged bridges for instance have a FilterMode None but a static alpha of 0 for some materials
				if (layer_colors[lay_index].a <= 0.01f) {
					lay_index += 1;
					continue;
				}

				if (j.hd != render_hd) {
					lay_index += 1;
					continue;
				}

				glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.0f);
				glUniform1i(5, lay_index);

				switch (j.blend_mode) {
					case 0:
					case 1:
						glBlendFunc(GL_ONE, GL_ZERO);
						break;
					case 2:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						break;
					case 3:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						break;
					case 4:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						break;
					case 5:
						glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						break;
					case 6:
						glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
						break;
				}

				glUniform1i(2, !(j.shading_flags & 0x1) && render_lighting);

				if (j.shading_flags & 0x10) {
					glDisable(GL_CULL_FACE);
				} else {
					glEnable(GL_CULL_FACE);
				}

				if (j.shading_flags & 0x40) {
					glDisable(GL_DEPTH_TEST);
				} else {
					glEnable(GL_DEPTH_TEST);
				}

				if (j.shading_flags & 0x80) {
					glDepthMask(false);
				} else {
					glDepthMask(true);
				}

				for (size_t texture_slot = 0; texture_slot < j.texturess.size(); texture_slot++) {
					glBindTextureUnit(texture_slot, textures[j.texturess[texture_slot].id]->id);
				}

				glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
				lay_index += 1;
			}
		}
	}

	void render_transparent(int instance_id, bool render_hd, bool render_lighting) {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(vao);

		glUniform1i(4, instance_id);
		glUniform1i(6, skip_count);

		glUniform1ui(9, instance_vertex_count);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uv_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, preskinned_vertex_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, preskinned_tangent_light_direction_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normal_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, instance_ssbo);

		int lay_index = 0;
		for (const auto& i : geosets) {
			const auto& layers = model->materials[i.material_id].layers;

			if (layers[0].blend_mode == 0 || layers[0].blend_mode == 1) {
				lay_index += layers.size();
				continue;
			}

			for (const auto& j : layers) {
				// We don't have to render fully transparent meshes
				if (layer_colors[instance_id * skip_count + lay_index].a <= 0.01f) {
					lay_index += 1;
					continue;
				}

				if (j.hd != render_hd) {
					lay_index += 1;
					continue;
				}

				glUniform1i(7, lay_index);

				switch (j.blend_mode) {
					case 2:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						break;
					case 3:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						break;
					case 4:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						break;
					case 5:
						glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						break;
					case 6:
						glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
						break;
				}

				glUniform1i(2, !(j.shading_flags & 0x1) && render_lighting);

				if (j.shading_flags & 0x10) {
					glDisable(GL_CULL_FACE);
				} else {
					glEnable(GL_CULL_FACE);
				}

				if (j.shading_flags & 0x40) {
					glDisable(GL_DEPTH_TEST);
				} else {
					glEnable(GL_DEPTH_TEST);
				}

				for (size_t texture_slot = 0; texture_slot < j.texturess.size(); texture_slot++) {
					glBindTextureUnit(texture_slot, textures[j.texturess[texture_slot].id]->id);
				}

				glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
				lay_index += 1;
			}
		}
	}

	void render_color_coded(const SkeletalModelInstance& skeleton, int id) {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(vao);

		glm::mat4 MVP = camera.projection_view * skeleton.matrix;
		glUniformMatrix4fv(0, 1, false, &MVP[0][0]);

		glUniform1i(7, id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bones_ssbo_colored);
		glNamedBufferData(bones_ssbo_colored, model->bones.size() * sizeof(glm::mat4), skeleton.world_matrices.data(), GL_DYNAMIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertex_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, weight_buffer);

		for (const auto& i : geosets) {
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeleton.sequence_index >= 0) {
				geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*i.geoset_anim);
			}

			for (const auto& j : model->materials[i.material_id].layers) {
				float layer_visibility = 1.0f;
				if (skeleton.sequence_index >= 0) {
					layer_visibility = skeleton.get_layer_visiblity(j);
				}

				float final_visibility = layer_visibility * geoset_anim_visibility;
				if (final_visibility <= 0.001f) {
					continue;
				}

				if (j.shading_flags & 0x40) {
					glDisable(GL_DEPTH_TEST);
				} else {
					glEnable(GL_DEPTH_TEST);
				}

				if (j.shading_flags & 0x80) {
					glDepthMask(false);
				} else {
					glDepthMask(true);
				}

				if (j.shading_flags & 0x10) {
					glDisable(GL_CULL_FACE);
				} else {
					glEnable(GL_CULL_FACE);
				}

				glDrawElementsBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), i.base_vertex);
				break;
			}
		}
	}
};