module;

#include <cassert>
#include <cstdint>
#include <volk.h>
#include <vk_mem_alloc.h>

export module SkinnedMeshGlobals;

import std;
import VkContext;
import VkResources;

/// Shared mega-buffers for all SkinnedMesh static data. Per-frame data lives in the frame allocator.
/// reserve() is mutex-protected so worker threads can carve out non-overlapping ranges and upload into them.
export class SkinnedMeshGlobals {
  public:
	struct Allocation {
		uint32_t vertex_base;
		uint32_t index_base;
		uint32_t layer_base;
	};

	// Mirrors std430 layout in data/shaders/skinned_mesh_common.glsl - 32 bytes.
	struct DrawInfo {
		uint32_t instance_offset;
		uint32_t bone_offset;
		uint32_t bone_count;
		uint32_t layer_color_offset;
		uint32_t layer_skip_count;
		uint32_t layer_index_global;
		uint32_t layer_index_local;
		uint32_t _pad;
	};

	using DrawIndexedIndirectCommand = VkDrawIndexedIndirectCommand;

	/// A copy of CPU data into part of one of the static buffers
	struct Upload {
		const Buffer* buffer;
		VkDeviceSize offset;
		std::span<const std::byte> data;
	};

	// Capacity sized large enough for any reasonable Warcraft III map. Reserve throws on overflow.
	static constexpr uint32_t max_vertices = 4 * 1024 * 1024;
	static constexpr uint32_t max_indices = 12 * 1024 * 1024;
	static constexpr uint32_t max_layers = 4 * 1024;

	// Static buffers, created in init() and filled by SkinnedMesh constructors
	Buffer vertex_snorm_buffer;
	Buffer uv_snorm_buffer;
	Buffer normal_buffer;
	Buffer tangent_buffer;
	Buffer weight_buffer;
	Buffer index_buffer;
	Buffer layer_texture_ids_buffer;
	Buffer layer_params_buffer;

	/// Must run before any SkinnedMesh constructor
	void init() {
		if (vertex_snorm_buffer.buffer != VK_NULL_HANDLE) {
			return;
		}
		constexpr VkBufferUsageFlags storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertex_snorm_buffer = create_buffer(max_vertices * sizeof(uint64_t), storage, false);
		uv_snorm_buffer = create_buffer(max_vertices * sizeof(uint32_t), storage, false);
		normal_buffer = create_buffer(max_vertices * sizeof(uint32_t), storage, false);
		tangent_buffer = create_buffer(max_vertices * 16, storage, false);
		weight_buffer = create_buffer(max_vertices * 16, storage, false);
		index_buffer = create_buffer(max_indices * sizeof(uint16_t), storage | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false);
		layer_texture_ids_buffer = create_buffer(max_layers * 32, storage, false);
		layer_params_buffer = create_buffer(max_layers * 16, storage, false);
	}

	Allocation reserve(const uint32_t vertices, const uint32_t indices, const uint32_t layers) {
		std::lock_guard lock(reserve_mutex);

		if (vertex_top + vertices > max_vertices || index_top + indices > max_indices || layer_top + layers > max_layers) {
			std::println("vertex_top {}\nindex_top {}\nlayer_top {}", vertex_top, index_top, layer_top);
			throw std::runtime_error("SkinnedMeshGlobals: vertex, index, or layer capacity exceeded");
		}

		Allocation a {vertex_top, index_top, layer_top};
		vertex_top += vertices;
		index_top += indices;
		layer_top += layers;

		return a;
	}

	void reset() {
		std::lock_guard lock(reserve_mutex);
		vertex_top = 0;
		index_top = 0;
		layer_top = 0;
	}

	/// Copies all uploads with one staging buffer and one submission. Thread-safe; blocks until done.
	/// Waits for earlier GPU work first, since ranges freed by reset() may still be read by frames in flight.
	static void upload(const std::span<const Upload> uploads) {
		VkDeviceSize total = 0;
		for (const auto& upload : uploads) {
			total += (upload.data.size() + 15) / 16 * 16;
		}
		if (total == 0) {
			return;
		}

		Buffer staging = create_buffer(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
		std::vector<VkDeviceSize> staging_offsets;
		VkDeviceSize offset = 0;
		for (const auto& upload : uploads) {
			std::memcpy(static_cast<std::byte*>(staging.mapped) + offset, upload.data.data(), upload.data.size());
			staging_offsets.push_back(offset);
			offset += (upload.data.size() + 15) / 16 * 16;
		}

		vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
			transfer_after_reads_barrier(cmd);
			for (size_t i = 0; i < uploads.size(); i++) {
				if (uploads[i].data.empty()) {
					continue;
				}
				const VkBufferCopy region = {staging_offsets[i], uploads[i].offset, uploads[i].data.size()};
				vkCmdCopyBuffer(cmd, staging.buffer, uploads[i].buffer->buffer, 1, &region);
			}
			transfer_write_barrier(cmd);
		});
		vmaDestroyBuffer(vk_context.allocator, staging.buffer, staging.allocation);
	}

  private:
	std::mutex reserve_mutex;
	uint32_t vertex_top = 0;
	uint32_t index_top = 0;
	uint32_t layer_top = 0;
};

export inline SkinnedMeshGlobals skinned_mesh_globals;
