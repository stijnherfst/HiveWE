module;

#include <volk.h>

export module VkEditableMesh;

import std;
import MDX;
import Skeleton;
import ResourceManager;
import ParticleEmitter2Renderer;
import VkContext;
import VkResources;
import VkTexture;
import <glm/glm.hpp>;

namespace fs = std::filesystem;

export struct MeshTextureRequest {
	fs::path path;
	int flags;
};

/// The texture file for each entry in mdx.textures, in order, resolving replaceable IDs and empty file names
export std::vector<MeshTextureRequest>
mesh_texture_requests(const mdx::MDX& mdx, const std::optional<std::pair<int, std::string>>& replaceable_id_override) {
	std::vector<MeshTextureRequest> requests;
	requests.reserve(mdx.textures.size());

	for (size_t i = 0; i < mdx.textures.size(); i++) {
		const mdx::Texture& texture = mdx.textures[i];
		const int flags = static_cast<int>(texture.flags);

		if (texture.replaceable_id != 0) {
			// Figure out if this is an HD texture
			// Unfortunately replaceable ID textures don't have any additional information on whether they are diffuse/normal/orm
			// So we take a guess using the index
			std::string suffix("");
			bool found = false;
			for (const auto& material : mdx.materials) {
				for (const auto& layer : material.layers) {
					for (size_t j = 0; j < layer.textures.size(); j++) {
						if (layer.textures[j].id != i) {
							continue;
						}

						found = true;

						if (mdx::is_hd_shader(layer.shader)) {
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
				requests.push_back({replaceable_id_override->second + suffix, flags});
			} else {
				requests.push_back({mdx::replaceable_id_to_texture.at(texture.replaceable_id) + suffix, flags});
			}
		} else if (texture.file_name.empty()) {
			// An empty filename means no texture/pure white.
			requests.push_back({"textures/white.dds", flags});
		} else {
			requests.push_back({texture.file_name, flags});
		}
	}
	return requests;
}

/// Written to the frame allocator once per model per frame, followed by the bone matrices.
/// Mirrors MeshFrameData in data/shaders/editable_mesh_*.
export struct MeshFrameData {
	glm::mat4 mvp;
	glm::vec4 light_direction;
	uint32_t team_color_index;
	uint32_t padding[3];
};
static_assert(sizeof(MeshFrameData) == 96);

/// Mirrors PushConstants in data/shaders/editable_mesh_*
export struct MeshPushConstants {
	VkDeviceAddress frame;
	uint32_t texture_slots[5];
	uint32_t flags;
	glm::vec4 layer_color;
	float alpha_test;
	/// The mesh's vertex streams, see VulkanEditableMesh::buffer
	VkDeviceAddress positions;
	VkDeviceAddress uvs;
	VkDeviceAddress normals;
	VkDeviceAddress tangents;
	VkDeviceAddress skins;
};
static_assert(offsetof(MeshPushConstants, texture_slots) == 8);
static_assert(offsetof(MeshPushConstants, flags) == 28);
static_assert(offsetof(MeshPushConstants, layer_color) == 32);
static_assert(offsetof(MeshPushConstants, alpha_test) == 48);
static_assert(offsetof(MeshPushConstants, positions) == 56);
static_assert(sizeof(MeshPushConstants) <= Bindless::push_constant_size);

/// Mirrors PushConstants in data/shaders/particle_emitter2.*
export struct ParticlePushConstants {
	glm::mat4 mvp;
	uint32_t texture_slot;
	int32_t filter_mode;
	VkDeviceAddress vertices;
};
static_assert(offsetof(ParticlePushConstants, vertices) == 72);

/// The distinct glBlendFunc settings MDX layers and particle emitters use
export enum BlendMode {
	blend_none,		  // ONE, ZERO
	blend_alpha,	  // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
	blend_additive,	  // SRC_ALPHA, ONE
	blend_modulate,	  // ZERO, SRC_COLOR
	blend_modulate2x, // DST_COLOR, SRC_COLOR
	blend_mode_count
};

export struct BlendFactors {
	bool enable;
	VkBlendFactor src;
	VkBlendFactor dst;
};

export constexpr std::array<BlendFactors, blend_mode_count> blend_factors = {{
	{false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO},
	{true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
	{true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE},
	{true, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_SRC_COLOR},
	{true, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_SRC_COLOR},
}};

export BlendMode layer_blend_mode(const uint32_t mdx_blend_mode) {
	switch (mdx_blend_mode) {
		case 2:
			return blend_alpha;
		case 3:
		case 4:
			return blend_additive;
		case 5:
			return blend_modulate;
		case 6:
			return blend_modulate2x;
		default:
			return blend_none;
	}
}

export BlendMode particle_blend_mode(const uint32_t filter_mode) {
	switch (filter_mode) {
		case 1:
			return blend_additive;
		case 2:
			return blend_modulate;
		case 3:
			return blend_modulate2x;
		default: // Blend and AlphaKey; AlphaKey's discard happens in the shader
			return blend_alpha;
	}
}

namespace {
	constexpr uint32_t flag_lighting = 1;
	constexpr uint32_t flag_team_color = 2;

	template <typename T>
	void append_bytes(std::vector<std::byte>& bytes, const size_t offset, const T* data, const size_t count) {
		std::memcpy(bytes.data() + offset, data, count * sizeof(T));
	}
} // namespace

/// Vulkan counterpart of EditableMesh: LOD 0 geosets of one MDX in device-local memory plus its textures
export class VulkanEditableMesh {
  public:
	struct MeshEntry {
		uint32_t indices = 0;
		int32_t base_vertex = 0;
		uint32_t base_index = 0;
		uint32_t material_id = 0;
		mdx::GeosetAnimation* geoset_anim = nullptr; // can be nullptr, often
	};

	std::shared_ptr<mdx::MDX> mdx;
	std::vector<MeshEntry> geosets;
	std::vector<std::shared_ptr<VulkanTexture>> textures;

	/// Positions, UVs, normals, tangents, skin weights and indices, each stream packed back to back
	Buffer buffer;
	std::array<VkDeviceSize, 5> stream_offsets {};
	VkDeviceSize index_offset = 0;

	explicit VulkanEditableMesh(std::shared_ptr<mdx::MDX> mdx, const std::optional<std::pair<int, std::string>>& replaceable_id_override)
		: mdx(std::move(mdx)) {
		if (!this->mdx->is_valid()) {
			throw std::runtime_error("Mesh has severe errors and cannot be rendered. Check them in the model editor.");
		}

		this->mdx->fix_up();

		size_t vertices = 0;
		size_t indices = 0;
		for (const auto& geoset : this->mdx->geosets) {
			if (geoset.lod != 0) {
				continue;
			}
			vertices += geoset.vertices.size();
			indices += geoset.faces.size();
		}

		// Stream strides; skin weights are 4 bone indices then 4 weights, 16 bits each
		constexpr std::array<size_t, 5> strides = {sizeof(glm::vec3), sizeof(glm::vec2), sizeof(glm::vec3), sizeof(glm::vec4), 16};
		size_t total = 0;
		for (size_t i = 0; i < strides.size(); i++) {
			stream_offsets[i] = total;
			total += vertices * strides[i];
		}
		index_offset = total;
		total += indices * sizeof(uint16_t);

		// Missing tangents stay zero
		std::vector<std::byte> bytes(std::max<size_t>(total, 4));

		uint32_t base_vertex = 0;
		uint32_t base_index = 0;
		for (const auto& geoset : this->mdx->geosets) {
			if (geoset.lod != 0) {
				continue;
			}
			const size_t count = geoset.vertices.size();
			geosets.push_back({
				.indices = static_cast<uint32_t>(geoset.faces.size()),
				.base_vertex = static_cast<int32_t>(base_vertex),
				.base_index = base_index,
				.material_id = geoset.material_id,
			});

			append_bytes(bytes, stream_offsets[0] + base_vertex * strides[0], geoset.vertices.data(), count);
			append_bytes(bytes, stream_offsets[1] + base_vertex * strides[1], geoset.uv_sets.front().data(), count);
			append_bytes(bytes, stream_offsets[2] + base_vertex * strides[2], geoset.normals.data(), count);
			if (!geoset.tangents.empty()) {
				append_bytes(bytes, stream_offsets[3] + base_vertex * strides[3], geoset.tangents.data(), count);
			}
			// SD models store matrix groups; they are converted to HD skin weights (at most 4 bones per vertex, like HD)
			if (geoset.skin.empty()) {
				const std::vector<glm::u16vec4> skin_weights = mdx::MDX::matrix_groups_as_skin_weights(geoset);
				append_bytes(bytes, stream_offsets[4] + base_vertex * strides[4], skin_weights.data(), count * 2);
			} else {
				append_bytes(bytes, stream_offsets[4] + base_vertex * strides[4], geoset.skin.data(), count * 8);
			}
			append_bytes(bytes, index_offset + base_index * sizeof(uint16_t), geoset.faces.data(), geoset.faces.size());

			base_vertex += static_cast<uint32_t>(count);
			base_index += static_cast<uint32_t>(geoset.faces.size());
		}

		// animations geoset ids > geosets
		for (auto& animation : this->mdx->animations) {
			if (animation.geoset_id < geosets.size()) {
				geosets[animation.geoset_id].geoset_anim = &animation;
			}
		}

		buffer = create_device_buffer(bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

		for (const auto& [path, flags] : mesh_texture_requests(*this->mdx, replaceable_id_override)) {
			textures.push_back(resource_manager.load<VulkanTexture>(path, std::to_string(flags), flags).value());
		}
	}

	VulkanEditableMesh(const VulkanEditableMesh&) = delete;
	VulkanEditableMesh& operator=(const VulkanEditableMesh&) = delete;

	~VulkanEditableMesh() {
		destroy_buffer_deferred(buffer);
	}
};

/// Pipelines for drawing VulkanEditableMesh into one color + depth attachment format
export class EditableMeshRenderer {
	std::array<Pipeline, blend_mode_count> sd_pipelines;
	std::array<Pipeline, blend_mode_count> hd_pipelines;
	std::array<Pipeline, blend_mode_count> particle_pipelines;

	std::vector<ParticleVertex> particle_scratch;
	std::vector<std::pair<float, size_t>> particle_sort_scratch;

  public:
	EditableMeshRenderer() {
		for (size_t mode = 0; mode < blend_mode_count; mode++) {
			const auto& [enable, src, dst] = blend_factors[mode];
			PipelineDescription description = {
				.blend = enable,
				.src_factor = src,
				.dst_factor = dst,
			};
			description.vertex_shader = "data/shaders/editable_mesh_sd.vert.spv";
			description.fragment_shader = "data/shaders/editable_mesh_sd.frag.spv";
			sd_pipelines[mode] = Pipeline(description);

			description.vertex_shader = "data/shaders/editable_mesh_hd.vert.spv";
			description.fragment_shader = "data/shaders/editable_mesh_hd.frag.spv";
			hd_pipelines[mode] = Pipeline(description);

			description.vertex_shader = "data/shaders/particle_emitter2.vert.spv";
			description.fragment_shader = "data/shaders/particle_emitter2.frag.spv";
			particle_pipelines[mode] = Pipeline(description);
		}
	}

	EditableMeshRenderer(const EditableMeshRenderer&) = delete;
	EditableMeshRenderer& operator=(const EditableMeshRenderer&) = delete;

	/// Draws opaque layers, then transparent layers, then particles, like EditableMesh + ParticleEmitter2Renderer.
	/// Expects the viewport to be set and the bindless set bound.
	void render(
		const VkCommandBuffer cmd,
		FrameAllocator& allocator,
		const VulkanEditableMesh& mesh,
		const Skeleton& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3& light_direction,
		const glm::vec3& camera_right,
		const glm::vec3& camera_up,
		const glm::vec3& camera_forward,
		const uint32_t team_color_index
	) {
		const mdx::MDX& mdx = *mesh.mdx;

		if (!mdx.geosets.empty()) {
			const size_t bone_count = mdx.bones.size();
			const auto frame_allocation = allocator.allocate(sizeof(MeshFrameData) + bone_count * sizeof(glm::mat4));
			const MeshFrameData frame = {
				.mvp = projection_view,
				.light_direction = glm::vec4(light_direction, 0.f),
				.team_color_index = team_color_index,
			};
			std::memcpy(frame_allocation.data, &frame, sizeof(frame));
			std::memcpy(frame_allocation.data + sizeof(frame), skeleton.world_matrices.data(), bone_count * sizeof(glm::mat4));

			vkCmdBindIndexBuffer(cmd, mesh.buffer.buffer, mesh.index_offset, VK_INDEX_TYPE_UINT16);

			render_pass(cmd, mesh, skeleton, frame_allocation.address, true, false);
			render_pass(cmd, mesh, skeleton, frame_allocation.address, true, true);
			render_pass(cmd, mesh, skeleton, frame_allocation.address, false, false);
			render_pass(cmd, mesh, skeleton, frame_allocation.address, false, true);
		}

		render_particles(cmd, allocator, mesh, skeleton, projection_view, camera_right, camera_up, camera_forward);
	}

  private:
	void render_pass(
		const VkCommandBuffer cmd,
		const VulkanEditableMesh& mesh,
		const Skeleton& skeleton,
		const VkDeviceAddress frame_address,
		const bool opaque_pass,
		const bool render_hd
	) const {
		const mdx::MDX& mdx = *mesh.mdx;
		const auto& pipelines = render_hd ? hd_pipelines : sd_pipelines;
		VkPipeline bound = VK_NULL_HANDLE;

		for (const auto& geoset : mesh.geosets) {
			const auto& layers = mdx.materials[geoset.material_id].layers;
			if (layers.empty()) {
				continue;
			}

			const bool geoset_is_opaque = layers[0].blend_mode == 0 || layers[0].blend_mode == 1;
			if (geoset_is_opaque != opaque_pass) {
				continue;
			}

			glm::vec3 geoset_color(1.f);
			float geoset_anim_visibility = 1.f;
			if (geoset.geoset_anim && skeleton.sequence_index >= 0) {
				geoset_color = skeleton.get_geoset_animation_color(*geoset.geoset_anim);
				geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*geoset.geoset_anim);
			}

			for (const auto& layer : layers) {
				if (mdx::is_hd_shader(layer.shader) != render_hd) {
					continue;
				}

				const float layer_visibility = skeleton.sequence_index >= 0 ? skeleton.get_layer_visiblity(layer) : 1.f;
				const float final_alpha = layer_visibility * geoset_anim_visibility;
				if (!opaque_pass && final_alpha <= 0.01f) {
					continue;
				}

				const VkPipeline pipeline = pipelines[layer_blend_mode(layer.blend_mode)];
				if (pipeline != bound) {
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
					bound = pipeline;
				}
				vkCmdSetCullMode(cmd, (layer.shading_flags & 0x10) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);
				vkCmdSetDepthTestEnable(cmd, !(layer.shading_flags & 0x40));
				vkCmdSetDepthWriteEnable(cmd, opaque_pass && !(layer.shading_flags & 0x80));

				MeshPushConstants push = {
					.frame = frame_address,
					.layer_color = glm::vec4(geoset_color, final_alpha),
					.alpha_test = layer.blend_mode == 1 ? 0.75f : 0.01f,
					.positions = mesh.buffer.address + mesh.stream_offsets[0],
					.uvs = mesh.buffer.address + mesh.stream_offsets[1],
					.normals = mesh.buffer.address + mesh.stream_offsets[2],
					.tangents = mesh.buffer.address + mesh.stream_offsets[3],
					.skins = mesh.buffer.address + mesh.stream_offsets[4],
				};
				// SD uses only the albedo slot. HD layers with fewer than 5 textures fall back to albedo.
				const uint32_t fallback_texture = layer.textures[0].id;
				for (size_t slot = 0; slot < (render_hd ? 5u : 1u); slot++) {
					const uint32_t texture = slot < layer.textures.size() ? layer.textures[slot].id : fallback_texture;
					push.texture_slots[slot] = mesh.textures[texture]->slot;
				}
				if (!(layer.shading_flags & 0x1)) {
					push.flags |= flag_lighting;
				}
				const uint32_t replaceable = mdx.textures[fallback_texture].replaceable_id;
				if (replaceable == 1 || replaceable == 2) {
					push.flags |= flag_team_color;
				}
				vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

				vkCmdDrawIndexed(cmd, geoset.indices, 1, geoset.base_index, geoset.base_vertex, 0);
			}
		}
	}

	void render_particles(
		const VkCommandBuffer cmd,
		FrameAllocator& allocator,
		const VulkanEditableMesh& mesh,
		const Skeleton& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3& camera_right,
		const glm::vec3& camera_up,
		const glm::vec3& camera_forward
	) {
		const mdx::MDX& mdx = *mesh.mdx;
		bool state_set = false;

		for (size_t i = 0; i < mdx.emitters2.size(); i++) {
			const mdx::ParticleEmitter2& emitter = mdx.emitters2[i];
			const auto& pool = skeleton.particles.pools[i];
			if (pool.alive_count == 0) {
				continue;
			}

			particle_scratch.clear();
			build_emitter_vertices(
				emitter,
				pool,
				emitter_world_matrix(mdx, skeleton, emitter),
				camera_right,
				camera_up,
				camera_forward,
				particle_scratch,
				particle_sort_scratch
			);
			if (particle_scratch.empty()) {
				continue;
			}

			if (!state_set) {
				vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
				vkCmdSetDepthTestEnable(cmd, VK_TRUE);
				vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
				state_set = true;
			}

			const auto vertices = allocator.upload(std::span<const ParticleVertex>(particle_scratch), 16);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particle_pipelines[particle_blend_mode(emitter.filter_mode)]);

			const ParticlePushConstants push = {
				.mvp = projection_view,
				.texture_slot = mesh.textures[emitter.texture_id]->slot,
				.filter_mode = static_cast<int32_t>(emitter.filter_mode),
				.vertices = vertices.address,
			};
			vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
			vkCmdDraw(cmd, static_cast<uint32_t>(particle_scratch.size()), 1, 0, 0);
		}
	}
};
