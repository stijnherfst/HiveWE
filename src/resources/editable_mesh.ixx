export module EditableMesh;

import std;
import MDX;
import ResourceManager;
import GPUTexture;
import Shader;
import SkeletalModelInstance;
import Hierarchy;
import BinaryReader;
import <glm/glm.hpp>;
import <glad/glad.h>;

namespace fs = std::filesystem;

export class EditableMesh: public Resource {
  public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		mdx::Extent extent;

		bool hd = true;
		mdx::GeosetAnimation* geoset_anim; // can be nullptr, often
	};

	std::shared_ptr<mdx::MDX> mdx;

	std::vector<MeshEntry> geosets;
	bool has_mesh; // ToDo remove when added support for meshless

	GLuint vao;
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint tangent_buffer;
	GLuint weight_buffer;
	GLuint index_buffer;
	GLuint layer_alpha;
	GLuint geoset_color;

	std::vector<std::shared_ptr<GPUTexture>> textures;

	static constexpr const char* name = "EditableMesh";

	EditableMesh() = delete;

	explicit EditableMesh(std::shared_ptr<mdx::MDX> mdx, std::optional<std::pair<int, std::string>> replaceable_id_override) {
		this->mdx = mdx;

		size_t vertices = 0;
		size_t indices = 0;
		size_t matrices = 0;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		has_mesh = mdx->geosets.size();
		if (!has_mesh) {
			return;
		}

		// Calculate required space
		for (const auto& i : mdx->geosets) {
			if (i.lod != 0) {
				continue;
			}
			vertices += i.vertices.size();
			indices += i.faces.size();
			matrices += i.matrix_groups.size();
		}

		// Allocate space
		glCreateBuffers(1, &vertex_buffer);
		glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &uv_buffer);
		glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &normal_buffer);
		glNamedBufferData(normal_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &tangent_buffer);
		glNamedBufferData(tangent_buffer, vertices * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &weight_buffer);
		glNamedBufferData(weight_buffer, vertices * sizeof(glm::uvec2), nullptr, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &index_buffer);
		glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

		// Buffer Data
		int base_vertex = 0;
		int base_index = 0;

		for (const auto& i : mdx->geosets) {
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
			// This could cause graphical inconsistensies with the game, but after more than 4 bones the contribution per bone is low enough that we don't care
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

			glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
			glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.uv_sets.front().data());
			glNamedBufferSubData(normal_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.normals.data());
			glNamedBufferSubData(tangent_buffer, base_vertex * sizeof(glm::vec4), entry.vertices * sizeof(glm::vec4), i.tangents.data());
			glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

			base_vertex += entry.vertices;
			base_index += entry.indices;
		}

		// animations geoset ids > geosets
		for (auto& i : mdx->animations) {
			if (i.geoset_id < geosets.size()) {
				geosets[i.geoset_id].geoset_anim = &i;
			}
		}

		for (size_t i = 0; i < mdx->textures.size(); i++) {
			const mdx::Texture& texture = mdx->textures[i];

			if (texture.replaceable_id != 0) {
				// Figure out if this is an HD texture
				// Unfortunately replaceable ID textures don't have any additional information on whether they are diffuse/normal/orm
				// So we take a guess using the index
				std::string suffix("");
				bool found = false;
				for (const auto& material : mdx->materials) {
					for (const auto& layer : material.layers) {
						for (size_t j = 0; j < layer.textures.size(); j++) {
							if (layer.textures[j].id != i) {
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
					textures.push_back(
						resource_manager.load<GPUTexture>(replaceable_id_override->second + suffix, std::to_string(texture.flags))
					);
				} else {
					textures.push_back(resource_manager.load<GPUTexture>(
						mdx::replaceable_id_to_texture.at(texture.replaceable_id) + suffix,
						std::to_string(texture.flags)
					));
				}
			} else {
				textures.push_back(resource_manager.load<GPUTexture>(texture.file_name, std::to_string(texture.flags)));
			}
			glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, texture.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, texture.flags & 2 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		}

		glEnableVertexArrayAttrib(vao, 0);
		glEnableVertexArrayAttrib(vao, 1);
		glEnableVertexArrayAttrib(vao, 2);
		glEnableVertexArrayAttrib(vao, 3);
		glEnableVertexArrayAttrib(vao, 4);

		glVertexArrayElementBuffer(vao, index_buffer);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, weight_buffer);
		glVertexAttribIPointer(4, 2, GL_UNSIGNED_INT, 0, nullptr);
	}

	virtual ~EditableMesh() {
		glDeleteBuffers(1, &vertex_buffer);
		glDeleteBuffers(1, &uv_buffer);
		glDeleteBuffers(1, &normal_buffer);
		glDeleteBuffers(1, &tangent_buffer);
		glDeleteBuffers(1, &weight_buffer);
		glDeleteBuffers(1, &index_buffer);
		glDeleteBuffers(1, &layer_alpha);
		glDeleteBuffers(1, &geoset_color);
	}

	void render(
		const int team_color_index,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3 light_direction
	) const {
		render_opaque(false, team_color_index, skeleton, projection_view, light_direction);
	}

  private:
	void render_opaque(
		const bool render_hd,
		const int team_color_index,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3 light_direction
	) const {
		if (!has_mesh) {
			return;
		}
		glm::mat4 M = glm::mat4(1.f);
		glm::mat4 MVP = projection_view * M;

		glBindVertexArray(vao);

		glUniformMatrix4fv(0, 1, false, &MVP[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniformMatrix4fv(4, 1, false, &M[0][0]);
		glUniform1i(6, mdx->bones.size());
		glUniform1i(9, team_color_index);
		glUniformMatrix4fv(11, mdx->bones.size(), false, &skeleton.world_matrices[0][0][0]);

		for (const auto& i : geosets) {
			const auto& layers = mdx->materials[i.material_id].layers;

			if (layers[0].blend_mode != 0 && layers[0].blend_mode != 1) {
				continue;
			}

			glm::vec3 geoset_color(1.f);
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeleton.sequence_index >= 0) {
				geoset_color = skeleton.get_geoset_animation_color(*i.geoset_anim);
				geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*i.geoset_anim);
			}

			for (const auto& j : layers) {
				if (j.hd != render_hd) {
					continue;
				}

				float layer_visibility = 1.0f;
				if (skeleton.sequence_index >= 0) {
					layer_visibility = skeleton.get_layer_visiblity(j);
				}

				const glm::vec4 layer_color = glm::vec4(geoset_color, layer_visibility * geoset_anim_visibility);

				// We don't have to render fully transparent meshes
				// Some Reforged bridges for instance have a FilterMode None but a static alpha of 0 for some materials
				// TODO: this is a hack, if the first instance has an alpha animation that triggers this condition then all instances will be hidden, even if the others have a positive alpha
				if (layer_color.a <= 0.01f) {
					continue;
				}

				glUniform1f(1, j.blend_mode == 1 ? 0.75f : -1.0f);
				glUniform1i(2, !(j.shading_flags & 0x1));
				glUniform4fv(8, 1, &layer_color[0]);
				const bool is_team_color =
					(mdx->textures[j.textures[0].id].replaceable_id == 1 || mdx->textures[j.textures[0].id].replaceable_id == 2);
				glUniform1i(10, is_team_color);

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

				for (size_t texture_slot = 0; texture_slot < j.textures.size(); texture_slot++) {
					glBindTextureUnit(texture_slot, textures[j.textures[texture_slot].id]->id);
				}

				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					i.indices,
					GL_UNSIGNED_SHORT,
					reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)),
					i.base_vertex
				);
			}
		}
	}
};
