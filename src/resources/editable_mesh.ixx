export module EditableMesh;

import std;
import MDX;
import ResourceManager;
import GPUTexture;
import Shader;
import SkeletalModelInstance;
import ParticleEmitter2Renderer;
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

	GLuint vao = 0;
	GLuint vertex_buffer = 0;
	GLuint uv_buffer = 0;
	GLuint normal_buffer = 0;
	GLuint tangent_buffer = 0;
	GLuint weight_buffer = 0;
	GLuint index_buffer = 0;
	GLuint layer_alpha = 0;
	GLuint geoset_color = 0;

	std::vector<std::shared_ptr<GPUTexture>> textures;

	mutable ParticleEmitter2Renderer particle_renderer;

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
						resource_manager.load<GPUTexture>(replaceable_id_override->second + suffix, std::to_string(texture.flags), static_cast<int>(texture.flags)).value()
					);
				} else {
					textures.push_back(resource_manager.load<GPUTexture>(
						mdx::replaceable_id_to_texture.at(texture.replaceable_id) + suffix,
						std::to_string(texture.flags),
						static_cast<int>(texture.flags)
					).value());
				}
			} else {
				textures.push_back(resource_manager.load<GPUTexture>(texture.file_name, std::to_string(texture.flags), static_cast<int>(texture.flags)).value());
			}
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

		particle_renderer.init();
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

	void render_opaque(
		const bool render_hd,
		const int team_color_index,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3 light_direction
	) const {
		render_pass(true, render_hd, team_color_index, skeleton, projection_view, light_direction);
	}

	void render_transparent(
		const bool render_hd,
		const int team_color_index,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3 light_direction
	) const {
		render_pass(false, render_hd, team_color_index, skeleton, projection_view, light_direction);
	}

	void render_particles(
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3& camera_right,
		const glm::vec3& camera_up,
		const glm::vec3& camera_forward
	) const {
		particle_renderer.render(*mdx, textures, skeleton, projection_view, camera_right, camera_up, camera_forward);
	}

  private:
	void render_pass(
		const bool opaque_pass,
		const bool render_hd,
		const int team_color_index,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3 light_direction
	) const {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(vao);

		glUniformMatrix4fv(0, 1, false, &projection_view[0][0]);
		glUniform3fv(3, 1, &light_direction.x);
		glUniform1i(9, team_color_index);
		glUniformMatrix4fv(11, mdx->bones.size(), false, &skeleton.world_matrices[0][0][0]);

		for (const auto& geoset_entry : geosets) {
			const auto& layers = mdx->materials[geoset_entry.material_id].layers;
			const bool geoset_is_opaque = (layers[0].blend_mode == 0 || layers[0].blend_mode == 1);
			if (geoset_is_opaque != opaque_pass) {
				continue;
			}

			glm::vec3 geoset_color(1.f);
			float geoset_anim_visibility = 1.0f;
			if (geoset_entry.geoset_anim && skeleton.sequence_index >= 0) {
				geoset_color = skeleton.get_geoset_animation_color(*geoset_entry.geoset_anim);
				geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*geoset_entry.geoset_anim);
			}

			for (const auto& layer : layers) {
				if (layer.hd != render_hd) {
					continue;
				}

				float layer_visibility = 1.0f;
				if (skeleton.sequence_index >= 0) {
					layer_visibility = skeleton.get_layer_visiblity(layer);
				}

				const float final_alpha = layer_visibility * geoset_anim_visibility;
				if (!opaque_pass && final_alpha <= 0.01f) {
					continue;
				}

				const glm::vec4 layer_color = glm::vec4(geoset_color, final_alpha);

				glUniform1f(1, layer.blend_mode == 1 ? 0.75f : 0.01f);
				glUniform1i(2, !(layer.shading_flags & 0x1));
				glUniform4fv(8, 1, &layer_color[0]);

				const uint32_t replaceable = mdx->textures[layer.textures[0].id].replaceable_id;
				glUniform1i(10, replaceable == 1 || replaceable == 2);

				switch (layer.blend_mode) {
					case 0:
					case 1:
						glBlendFunc(GL_ONE, GL_ZERO);
						break;
					case 2:
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						break;
					case 3:
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

				if (layer.shading_flags & 0x10) {
					glDisable(GL_CULL_FACE);
				} else {
					glEnable(GL_CULL_FACE);
				}
				if (layer.shading_flags & 0x40) {
					glDisable(GL_DEPTH_TEST);
				} else {
					glEnable(GL_DEPTH_TEST);
				}
				if (opaque_pass) {
					glDepthMask(!(layer.shading_flags & 0x80));
				}

				// SD path: just albedo (slot 0). HD path: albedo/normal/orm/emissive/team_color (slots 0..4).
				// For HD layers with fewer than 5 textures, fall back to albedo so we don't read stale state.
				const size_t slot_count = render_hd ? 5 : 1;
				const uint32_t fallback_tex_id = layer.textures[0].id;
				for (size_t slot = 0; slot < slot_count; slot++) {
					const uint32_t tex_id = slot < layer.textures.size() ? layer.textures[slot].id : fallback_tex_id;
					glBindTextureUnit(static_cast<GLuint>(slot), textures[tex_id]->id);
				}

				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					geoset_entry.indices,
					GL_UNSIGNED_SHORT,
					reinterpret_cast<void*>(static_cast<uintptr_t>(geoset_entry.base_index) * sizeof(uint16_t)),
					geoset_entry.base_vertex
				);
			}
		}
	}
};
