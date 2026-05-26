export module SkinnedMesh;

import std;
import MDX;
import BinaryReader;
import ResourceManager;
import GPUTexture;
import Shader;
import Hierarchy;
import Timer;
import Camera;
import SkeletalModelInstance;
import SkinnedMeshGlobals;
import Utilities;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import <glm/gtc/packing.hpp>;
import <glad/glad.h>;

namespace fs = std::filesystem;

export class SkinnedMesh: public Resource {
  public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0; // mesh-local
		int base_index = 0; // mesh-local

		int material_id = 0;
		mdx::Extent extent;

		mdx::GeosetAnimation* geoset_anim; // can be nullptr, often
	};

	std::shared_ptr<mdx::MDX> mdx;

	std::vector<MeshEntry> geosets;
	bool has_mesh; // ToDo remove when added support for meshless
	bool has_transparent_layers = false;

	uint32_t instance_vertex_count = 0;

	// Offsets into the global SkinnedMeshGlobals buffers.
	uint32_t vertex_base = 0;
	uint32_t index_base = 0;
	uint32_t layer_base = 0;
	uint32_t texture_base = 0;

	// Per-mesh GPU buffer used only by the color-coded picking path.
	GLuint bones_ssbo_colored = 0;

	// Mirrors std430 layout in skinned_mesh_*.frag - 8 * uint = 32 bytes per entry.
	struct LayerTextureIds {
		uint32_t albedo; // also serves as SD diffuse
		uint32_t normal;
		uint32_t orm;
		uint32_t emissive;
		uint32_t team_color;
		uint32_t environment;
		uint32_t _pad0;
		uint32_t _pad1;
	};

	// Mirrors std430 layout in skinned_mesh_*.frag - 16 bytes per entry.
	struct LayerParams {
		float alpha_test;
		uint32_t layer_lit;
		uint32_t is_team_color;
		uint32_t _pad;
	};

	struct DrawState {
		GLenum src_factor;
		GLenum dst_factor;
		bool cull_face;
		bool depth_test;
		bool depth_mask;
		auto operator<=>(const DrawState&) const = default;
	};

	struct DrawEntry {
		DrawState state;
		uint32_t count;
		uint32_t first_index; // already global (includes index_base)
		int32_t base_vertex; // already global (includes vertex_base)
		uint32_t layer_index_global; // includes layer_base
		uint32_t layer_index_local; // mesh-local
	};

	std::vector<DrawEntry> opaque_entries_hd;
	std::vector<DrawEntry> opaque_entries_sd;
	std::vector<DrawEntry> transparent_entries_hd;
	std::vector<DrawEntry> transparent_entries_sd;

	int skip_count = 0;

	fs::path path;
	std::vector<std::shared_ptr<GPUTexture>> textures;
	bool textures_resident = false;

	std::vector<glm::mat4> render_jobs;
	std::vector<glm::vec3> render_colors;
	std::vector<uint32_t> render_team_color_indexes;
	std::vector<const SkeletalModelInstance*> skeletons;

	static constexpr const char* name = "SkinnedMesh";

	explicit SkinnedMesh(const fs::path& path, std::optional<std::pair<int, std::string>> replaceable_id_override) {
		fs::path new_path = path;
		new_path.replace_extension(".mdx");

		auto reader = hierarchy.open_file(new_path);
		if (reader) {
			mdx = std::make_shared<mdx::MDX>(reader.value());
		} else {
			new_path.replace_extension(".mdl");
			const auto reader = hierarchy.open_file(new_path).value();

			const auto view = std::string_view(reinterpret_cast<const char*>(reader.buffer.data()), reader.buffer.size());
			const auto result = mdx::MDX::from_mdl(view);
			mdx = std::make_shared<mdx::MDX>(std::move(result.value()));
		}

		this->path = new_path;

		size_t vertices = 0;
		size_t indices = 0;
		size_t matrices = 0;
		size_t total_layers = 0;

		has_mesh = mdx->geosets.size();
		if (!has_mesh) {
			return;
		}

		for (const auto& i : mdx->geosets) {
			const auto& layer = mdx->materials[i.material_id].layers[0];
			if (layer.blend_mode != 0 && layer.blend_mode != 1) {
				has_transparent_layers = true;
				break;
			}
		}

		// Calculate required space
		for (const auto& i : mdx->geosets) {
			if (i.lod != 0) {
				continue;
			}
			vertices += i.vertices.size();
			indices += i.faces.size();
			matrices += i.matrix_groups.size();
			total_layers += mdx->materials[i.material_id].layers.size();
		}

		// Reserve a contiguous range in each global mega-buffer. Mutex-protected; safe from worker threads.
		const auto alloc = skinned_mesh_globals.reserve(
			static_cast<uint32_t>(vertices),
			static_cast<uint32_t>(indices),
			static_cast<uint32_t>(total_layers),
			static_cast<uint32_t>(mdx->textures.size())
		);
		vertex_base = alloc.vertex_base;
		index_base = alloc.index_base;
		layer_base = alloc.layer_base;
		texture_base = alloc.texture_base;

		{
			ScopedTimer t(profile_gl_ns);
			glCreateBuffers(1, &bones_ssbo_colored);
		}

		// Buffer Data
		struct GeosetBuffers {
			std::vector<glm::u8vec4> skin_weights; // empty = use i.skin directly
			std::vector<glm::uvec2> vertices_snorm;
			std::vector<uint32_t> uvs_snorm;
			std::vector<uint32_t> normals_oct_snorm;
		};

		std::vector<GeosetBuffers> packed_geosets;

		// Pack vertices/uvs/normals
		{
			ScopedTimer t(profile_parse_ns);
			for (const auto& i : mdx->geosets) {
				if (i.lod != 0) {
					continue;
				}
				GeosetBuffers buf;

				if (i.skin.empty()) {
					buf.skin_weights = mdx::MDX::matrix_groups_as_skin_weights(i);
				}

				buf.vertices_snorm.reserve(i.vertices.size());
				for (const auto& j : i.vertices) {
					buf.vertices_snorm.push_back(pack_vec3_to_uvec2(j, 8192.f));
				}

				buf.uvs_snorm.reserve(i.uv_sets.front().size());
				for (const auto& j : i.uv_sets.front()) {
					buf.uvs_snorm.push_back(glm::packSnorm2x16((j + 1.f) / 8.f));
				}

				buf.normals_oct_snorm.reserve(i.normals.size());
				for (const auto& normal : i.normals) {
					buf.normals_oct_snorm.push_back(glm::packSnorm2x16(float32x3_to_oct(normal)));
				}

				packed_geosets.push_back(std::move(buf));
			}
		}

		// Upload
		int local_base_vertex = 0;
		int local_base_index = 0;
		int packed_index = 0;

		for (const auto& i : mdx->geosets) {
			if (i.lod != 0) {
				continue;
			}
			MeshEntry entry;
			entry.vertices = static_cast<int>(i.vertices.size());
			entry.base_vertex = local_base_vertex;

			entry.indices = static_cast<int>(i.faces.size());
			entry.base_index = local_base_index;

			entry.material_id = i.material_id;
			entry.geoset_anim = nullptr;
			entry.extent = i.extent;

			geosets.push_back(entry);

			const GeosetBuffers& buf = packed_geosets[packed_index++];
			const GLintptr v_off = vertex_base + local_base_vertex;
			const GLintptr i_off = index_base + local_base_index;

			if (i.skin.empty()) {
				glNamedBufferSubData(
					skinned_mesh_globals.weight_buffer,
					v_off * sizeof(glm::uvec2),
					entry.vertices * 8,
					buf.skin_weights.data()
				);
			} else {
				glNamedBufferSubData(skinned_mesh_globals.weight_buffer, v_off * sizeof(glm::uvec2), entry.vertices * 8, i.skin.data());
			}

			glNamedBufferSubData(
				skinned_mesh_globals.vertex_snorm_buffer,
				v_off * sizeof(glm::uvec2),
				entry.vertices * sizeof(glm::uvec2),
				buf.vertices_snorm.data()
			);

			glNamedBufferSubData(
				skinned_mesh_globals.uv_snorm_buffer,
				v_off * sizeof(uint32_t),
				entry.vertices * sizeof(uint32_t),
				buf.uvs_snorm.data()
			);

			glNamedBufferSubData(
				skinned_mesh_globals.normal_buffer,
				v_off * sizeof(uint32_t),
				entry.vertices * sizeof(uint32_t),
				buf.normals_oct_snorm.data()
			);

			if (!i.tangents.empty()) {
				glNamedBufferSubData(
					skinned_mesh_globals.tangent_buffer,
					v_off * sizeof(glm::vec4),
					entry.vertices * sizeof(glm::vec4),
					i.tangents.data()
				);
			}

			glNamedBufferSubData(
				skinned_mesh_globals.index_buffer,
				i_off * sizeof(uint16_t),
				entry.indices * sizeof(uint16_t),
				i.faces.data()
			);

			local_base_vertex += entry.vertices;
			local_base_index += entry.indices;
		}

		for (const auto& i : geosets) {
			skip_count += mdx->materials[i.material_id].layers.size();
			instance_vertex_count += i.indices;
		}

		// animations geoset ids > geosets
		for (auto& i : mdx->animations) {
			if (i.geoset_id >= 0 && i.geoset_id < geosets.size()) {
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

							if (layer.shader == mdx::ShaderType::HD) {
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
					textures.push_back(resource_manager
										   .load<GPUTexture>(
											   replaceable_id_override->second + suffix,
											   std::to_string(texture.flags),
											   static_cast<int>(texture.flags)
										   )
										   .value());
				} else {
					textures.push_back(resource_manager
										   .load<GPUTexture>(
											   mdx::replaceable_id_to_texture.at(texture.replaceable_id) + suffix,
											   std::to_string(texture.flags),
											   static_cast<int>(texture.flags)
										   )
										   .value());
				}
			} else {
				// An empty filename means no texture/pure white.
				if (texture.file_name.empty()) {
					textures.push_back(
						resource_manager
							.load<GPUTexture>("textures/white.dds", std::to_string(texture.flags), static_cast<int>(texture.flags))
							.value()
					);
				} else {
					textures.push_back(
						resource_manager.load<GPUTexture>(texture.file_name, std::to_string(texture.flags), static_cast<int>(texture.flags))
							.value()
					);
				}
			}
		}

		// Bindless texture handles into the global texture_handles SSBO.
		std::vector<GLuint64> handles;
		handles.reserve(textures.size());
		for (const auto& texture : textures) {
			handles.push_back(texture->bindless_handle);
		}

		glNamedBufferSubData(
			skinned_mesh_globals.texture_handles_ssbo,
			texture_base * sizeof(GLuint64),
			handles.size() * sizeof(GLuint64),
			handles.data()
		);

		// Layer texture-id and layer-param tables, written at this mesh's global slot.
		// Texture slots are pre-globalized so the fragment shader can index `textures[]` directly.
		std::vector<LayerTextureIds> layer_ids;
		layer_ids.reserve(total_layers);
		std::vector<LayerParams> layer_params;
		layer_params.reserve(total_layers);
		for (const auto& g : geosets) {
			for (const auto& layer : mdx->materials[g.material_id].layers) {
				LayerTextureIds e {};
				uint32_t* slots = &e.albedo;
				for (size_t s = 0; s < layer.textures.size() && s < 6; s++) {
					slots[s] = layer.textures[s].id + texture_base;
				}
				layer_ids.push_back(e);

				LayerParams p {};
				p.alpha_test = layer.blend_mode == 1 ? 0.75f : 0.01f;
				p.layer_lit = (layer.shading_flags & 0x1) ? 0u : 1u;
				const uint32_t replaceable_id = mdx->textures[layer.textures[0].id].replaceable_id;
				p.is_team_color = (replaceable_id == 1 || replaceable_id == 2) ? 1u : 0u;
				layer_params.push_back(p);
			}
		}

		glNamedBufferSubData(
			skinned_mesh_globals.layer_texture_ids_ssbo,
			layer_base * sizeof(LayerTextureIds),
			layer_ids.size() * sizeof(LayerTextureIds),
			layer_ids.data()
		);
		glNamedBufferSubData(
			skinned_mesh_globals.layer_params_ssbo,
			layer_base * sizeof(LayerParams),
			layer_params.size() * sizeof(LayerParams),
			layer_params.data()
		);

		// Pre-build per-layer indirect-draw entries. Walks geosets/layers in declaration order so the
		// global lay_index matches the layer_textures / layer_params buffers.
		auto blend_factors_for = [](const uint32_t blend_mode) -> std::pair<GLenum, GLenum> {
			switch (blend_mode) {
				case 2:
					return {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
				case 3:
					return {GL_SRC_ALPHA, GL_ONE};
				case 4:
					return {GL_SRC_ALPHA, GL_ONE};
				case 5:
					return {GL_ZERO, GL_SRC_COLOR};
				case 6:
					return {GL_DST_COLOR, GL_SRC_COLOR};
				default:
					return {GL_ONE, GL_ZERO};
			}
		};

		{
			int lay_index = 0;
			for (const auto& g : geosets) {
				const auto& layers = mdx->materials[g.material_id].layers;
				const bool geoset_is_opaque = (layers[0].blend_mode == 0 || layers[0].blend_mode == 1);

				for (const auto& layer : layers) {
					DrawEntry entry;
					const auto [src, dst] = blend_factors_for(layer.blend_mode);
					entry.state.src_factor = src;
					entry.state.dst_factor = dst;
					entry.state.cull_face = !(layer.shading_flags & 0x10);
					entry.state.depth_test = !(layer.shading_flags & 0x40);
					entry.state.depth_mask = !(layer.shading_flags & 0x80);
					entry.count = static_cast<uint32_t>(g.indices);
					entry.first_index = static_cast<uint32_t>(g.base_index + index_base);
					entry.base_vertex = static_cast<int32_t>(g.base_vertex + vertex_base);
					entry.layer_index_global = static_cast<uint32_t>(lay_index + layer_base);
					entry.layer_index_local = static_cast<uint32_t>(lay_index);

					const bool layer_is_hd = layer.shader == mdx::ShaderType::HD;
					auto& target = geoset_is_opaque ? (layer_is_hd ? opaque_entries_hd : opaque_entries_sd)
													: (layer_is_hd ? transparent_entries_hd : transparent_entries_sd);
					target.push_back(entry);

					lay_index += 1;
				}
			}

			std::ranges::sort(opaque_entries_hd, {}, &DrawEntry::state);
			std::ranges::sort(opaque_entries_sd, {}, &DrawEntry::state);
		}

		// Reclaim some space
		for (auto& i : mdx->geosets) {
			i.vertices.clear();
			i.vertices.shrink_to_fit();
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
			i.tangents.clear();
			i.tangents.shrink_to_fit();
			i.skin.clear();
			i.skin.shrink_to_fit();
			i.uv_sets.clear();
			i.uv_sets.shrink_to_fit();
		}
	}

	// Bindless handles must be made resident on the rendering context, but the textures may have been
	// loaded on a worker thread. Idempotent. Called from the main thread by RenderManager.
	void make_textures_resident() {
		if (textures_resident) {
			return;
		}
		for (const auto& tex : textures) {
			tex->make_resident();
		}
		textures_resident = true;
	}

	~SkinnedMesh() {
		glDeleteBuffers(1, &bones_ssbo_colored);
	}

	void clear_render_data() {
		render_jobs.clear();
		render_colors.clear();
		render_team_color_indexes.clear();
		skeletons.clear();
	}

	// Color-coded picking path: one mesh, one skeleton, per-geoset draw.
	void render_color_coded(const SkeletalModelInstance& skeleton, const int id) const {
		if (!has_mesh) {
			return;
		}

		glBindVertexArray(skinned_mesh_globals.vao);

		glm::mat4 MVP = camera.projection_view * skeleton.matrix;
		glUniformMatrix4fv(0, 1, false, &MVP[0][0]);

		glUniform1i(7, id);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bones_ssbo_colored);
		glNamedBufferData(bones_ssbo_colored, mdx->bones.size() * sizeof(glm::mat4), skeleton.world_matrices.data(), GL_DYNAMIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, skinned_mesh_globals.vertex_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skinned_mesh_globals.weight_buffer);

		for (const auto& i : geosets) {
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && skeleton.sequence_index >= 0) {
				geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*i.geoset_anim);
			}

			for (const auto& j : mdx->materials[i.material_id].layers) {
				float layer_visibility = 1.0f;
				if (skeleton.sequence_index >= 0) {
					layer_visibility = skeleton.get_layer_visiblity(j);
				}

				const float final_visibility = layer_visibility * geoset_anim_visibility;
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

				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					i.indices,
					GL_UNSIGNED_SHORT,
					reinterpret_cast<void*>((i.base_index + index_base) * sizeof(uint16_t)),
					i.base_vertex + vertex_base
				);
				break;
			}
		}
	}
};
