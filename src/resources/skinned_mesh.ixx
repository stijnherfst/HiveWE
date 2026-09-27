export module SkinnedMesh;

import std;
import BinaryReader;
import Camera;
import Hierarchy;
import MDX;
import ResourceManager;
import Skeleton;
import SkinnedMeshGlobals;
import Timer;
import Utilities;
import VkEditableMesh;
import VkTexture;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import <glm/gtc/packing.hpp>;

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
	bool has_transparent_layers = false;

	uint32_t instance_vertex_count = 0;

	// Offsets into the global SkinnedMeshGlobals buffers.
	uint32_t vertex_base = 0;
	uint32_t index_base = 0;
	uint32_t layer_base = 0;

	// Mirrors std430 layout in data/shaders/skinned_mesh_common.glsl - 8 * uint = 32 bytes per entry.
	// The values are bindless texture slots.
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

	// Mirrors std430 layout in data/shaders/skinned_mesh_common.glsl - 16 bytes per entry.
	struct LayerParams {
		float alpha_test;
		uint32_t layer_lit;
		uint32_t is_team_color;
		/// Bits 0-2: BlendMode, bit 3: two-sided
		uint32_t flags;
	};

	struct DrawState {
		/// Index into the renderer's pipelines for the MDX blend mode, see layer_blend_mode()
		uint8_t blend_mode;
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
	std::vector<std::shared_ptr<VulkanTexture>> textures;

	std::vector<glm::mat4> render_jobs;
	std::vector<glm::vec3> render_colors;
	std::vector<uint32_t> render_team_color_indexes;
	std::vector<const Skeleton*> skeletons;

	static constexpr const char* name = "SkinnedMesh";

	explicit SkinnedMesh(const fs::path& path, std::optional<std::pair<int, std::string>> replaceable_id_override) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"}).value();

		if (mdx::is_mdx(reader)) {
			mdx = std::make_shared<mdx::MDX>(reader);
		} else {
			const auto view = std::string_view(reinterpret_cast<const char*>(reader.buffer.data()), reader.buffer.size());
			const auto result = mdx::MDX::from_mdl(view);
			mdx = std::make_shared<mdx::MDX>(std::move(result.value()));
		}

		if (!mdx->is_valid()) {
			throw std::runtime_error(
				std::format("Mesh {} has severe errors and cannot be rendered. Check them in the model editor.", path.string())
			);
		}

		mdx->fix_up();

		this->path = path;

		size_t vertices = 0;
		size_t indices = 0;
		size_t matrices = 0;
		size_t total_layers = 0;

		for (const auto& i : mdx->geosets) {
			if (mdx->materials[i.material_id].layers.empty()) {
				continue;
			}
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
			static_cast<uint32_t>(total_layers)
		);
		vertex_base = alloc.vertex_base;
		index_base = alloc.index_base;
		layer_base = alloc.layer_base;

		// Everything this mesh puts in the static buffers, uploaded in one go at the end
		std::vector<SkinnedMeshGlobals::Upload> uploads;
		const auto& g = skinned_mesh_globals;

		// Buffer Data
		struct GeosetBuffers {
			std::vector<glm::u16vec4> skin_weights; // empty = use i.skin directly
			std::vector<glm::uvec2> vertices_snorm;
			std::vector<uint32_t> uvs_snorm;
			std::vector<uint32_t> normals_oct_snorm;
		};

		std::vector<GeosetBuffers> packed_geosets;

		// Pack vertices/uvs/normals
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
			const size_t v_off = vertex_base + local_base_vertex;
			const size_t i_off = index_base + local_base_index;

			// Skin weights are 4 bone indices then 4 weights, 16 bits each
			const auto skin = i.skin.empty() ? std::as_bytes(std::span(buf.skin_weights)) : std::as_bytes(std::span(i.skin));
			uploads.push_back({&g.weight_buffer, v_off * 16, skin.first(std::min<size_t>(skin.size(), entry.vertices * 16))});
			uploads.push_back({&g.vertex_snorm_buffer, v_off * sizeof(glm::uvec2), std::as_bytes(std::span(buf.vertices_snorm))});
			uploads.push_back({&g.uv_snorm_buffer, v_off * sizeof(uint32_t), std::as_bytes(std::span(buf.uvs_snorm))});
			uploads.push_back({&g.normal_buffer, v_off * sizeof(uint32_t), std::as_bytes(std::span(buf.normals_oct_snorm))});
			if (!i.tangents.empty()) {
				uploads.push_back({&g.tangent_buffer, v_off * sizeof(glm::vec4), std::as_bytes(std::span(i.tangents))});
			}
			uploads.push_back({&g.index_buffer, i_off * sizeof(uint16_t), std::as_bytes(std::span(i.faces))});

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

		for (const auto& [texture_path, flags] : mesh_texture_requests(*mdx, replaceable_id_override)) {
			textures.push_back(resource_manager.load<VulkanTexture>(texture_path, std::to_string(flags), flags).value());
		}

		// Layer texture-id and layer-param tables, written at this mesh's global slot.
		// Texture ids are bindless slots so the fragment shader can index `textures[]` directly.
		std::vector<LayerTextureIds> layer_ids;
		layer_ids.reserve(total_layers);
		std::vector<LayerParams> layer_params;
		layer_params.reserve(total_layers);
		for (const auto& g : geosets) {
			for (const auto& layer : mdx->materials[g.material_id].layers) {
				// Slots a layer has no texture for fall back to its albedo, so every slot the shaders read is valid
				LayerTextureIds e {};
				uint32_t* slots = &e.albedo;
				for (size_t s = 0; s < 6; s++) {
					const uint32_t texture = s < layer.textures.size() ? layer.textures[s].id : layer.textures[0].id;
					slots[s] = textures[texture]->slot;
				}
				layer_ids.push_back(e);

				LayerParams p {};
				p.alpha_test = layer.blend_mode == 1 ? 0.75f : 0.01f;
				p.layer_lit = (layer.shading_flags & 0x1) ? 0u : 1u;
				const uint32_t replaceable_id = mdx->textures[layer.textures[0].id].replaceable_id;
				p.is_team_color = (replaceable_id == 1 || replaceable_id == 2) ? 1u : 0u;
				p.flags = static_cast<uint32_t>(layer_blend_mode(layer.blend_mode)) | ((layer.shading_flags & 0x10) ? 8u : 0u);
				layer_params.push_back(p);
			}
		}

		uploads.push_back({&g.layer_texture_ids_buffer, layer_base * sizeof(LayerTextureIds), std::as_bytes(std::span(layer_ids))});
		uploads.push_back({&g.layer_params_buffer, layer_base * sizeof(LayerParams), std::as_bytes(std::span(layer_params))});
		SkinnedMeshGlobals::upload(uploads);

		// Pre-build per-layer indirect-draw entries. Walks geosets/layers in declaration order so the
		// global lay_index matches the layer_textures / layer_params buffers.
		{
			int lay_index = 0;
			for (const auto& g : geosets) {
				const auto& layers = mdx->materials[g.material_id].layers;
				if (layers.empty()) {
					continue;
				}

				const bool geoset_is_opaque = (layers[0].blend_mode == 0 || layers[0].blend_mode == 1);

				for (const auto& layer : layers) {
					DrawEntry entry;
					entry.state.blend_mode = static_cast<uint8_t>(layer_blend_mode(layer.blend_mode));
					entry.state.cull_face = !(layer.shading_flags & 0x10);
					entry.state.depth_test = !(layer.shading_flags & 0x40);
					entry.state.depth_mask = !(layer.shading_flags & 0x80);
					entry.count = static_cast<uint32_t>(g.indices);
					entry.first_index = static_cast<uint32_t>(g.base_index + index_base);
					entry.base_vertex = static_cast<int32_t>(g.base_vertex + vertex_base);
					entry.layer_index_global = static_cast<uint32_t>(lay_index + layer_base);
					entry.layer_index_local = static_cast<uint32_t>(lay_index);

					const bool layer_is_hd = mdx::is_hd_shader(layer.shader);
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

	~SkinnedMesh() override = default;

	void clear_render_data() {
		render_jobs.clear();
		render_colors.clear();
		render_team_color_indexes.clear();
		skeletons.clear();
	}

};
