module;

#include <volk.h>
#include <vk_mem_alloc.h>

export module VkResources;

import std;
import VkContext;

namespace fs = std::filesystem;

export struct Buffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	void* mapped = nullptr;
	VkDeviceAddress address = 0;
	VkDeviceSize size = 0;
};

/// Host-visible buffers are persistently mapped and meant for sequential CPU writes
export Buffer create_buffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const bool host_visible) {
	const VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	};
	VmaAllocationCreateInfo allocation_info = {.usage = VMA_MEMORY_USAGE_AUTO};
	if (host_visible) {
		allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	Buffer buffer;
	VmaAllocationInfo info;
	if (vmaCreateBuffer(vk_context.allocator, &buffer_info, &allocation_info, &buffer.buffer, &buffer.allocation, &info) != VK_SUCCESS) {
		throw std::runtime_error(std::format("Allocating a {} byte Vulkan buffer failed", size));
	}
	buffer.mapped = info.pMappedData;
	buffer.size = size;

	const VkBufferDeviceAddressInfo address_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer.buffer,
	};
	buffer.address = vkGetBufferDeviceAddress(vk_context.device.device, &address_info);
	return buffer;
}

export void destroy_buffer_deferred(const Buffer& buffer) {
	if (buffer.buffer == VK_NULL_HANDLE) {
		return;
	}
	vk_context.defer_destroy([buffer = buffer.buffer, allocation = buffer.allocation] {
		vmaDestroyBuffer(vk_context.allocator, buffer, allocation);
	});
}

/// Makes transfer writes recorded earlier in `cmd` visible to every later command on the queue
export void transfer_write_barrier(const VkCommandBuffer cmd) {
	const VkMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &barrier,
	};
	vkCmdPipelineBarrier2(cmd, &dependency);
}

/// Makes every earlier command on the queue finish reading before transfers recorded after this barrier write
export void transfer_after_reads_barrier(const VkCommandBuffer cmd) {
	const VkMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_NONE,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &barrier,
	};
	vkCmdPipelineBarrier2(cmd, &dependency);
}

/// Overwrites part of a device-local buffer that frames in flight may still be reading.
/// The copy waits for all earlier GPU work on the queue, and the call blocks until it is done.
export void upload_buffer(const Buffer& buffer, const std::span<const std::byte> data, const VkDeviceSize offset = 0) {
	if (data.empty()) {
		return;
	}
	Buffer staging = create_buffer(data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
	std::memcpy(staging.mapped, data.data(), data.size());
	vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
		transfer_after_reads_barrier(cmd);
		const VkBufferCopy region = {0, offset, data.size()};
		vkCmdCopyBuffer(cmd, staging.buffer, buffer.buffer, 1, &region);
		transfer_write_barrier(cmd);
	});
	vmaDestroyBuffer(vk_context.allocator, staging.buffer, staging.allocation);
}

export template <typename T>
void upload_buffer(const Buffer& buffer, const std::vector<T>& data) {
	upload_buffer(buffer, std::as_bytes(std::span(data)));
}

/// Creates a device-local buffer holding `data`. Blocks until the upload is done.
export Buffer create_device_buffer(const std::span<const std::byte> data, const VkBufferUsageFlags usage) {
	Buffer staging = create_buffer(data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
	std::memcpy(staging.mapped, data.data(), data.size());

	Buffer buffer = create_buffer(data.size(), usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
	vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
		const VkBufferCopy region = {0, 0, data.size()};
		vkCmdCopyBuffer(cmd, staging.buffer, buffer.buffer, 1, &region);
		transfer_write_barrier(cmd);
	});
	vmaDestroyBuffer(vk_context.allocator, staging.buffer, staging.allocation);
	return buffer;
}

export struct Image {
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkExtent2D extent = {0, 0};
	uint32_t mip_levels = 1;
	uint32_t layers = 1;
};

/// `array_view` gives the view type VK_IMAGE_VIEW_TYPE_2D_ARRAY (for sampler2DArray) even with a single layer
export Image create_image(
	const VkFormat format,
	const VkExtent2D extent,
	const uint32_t mip_levels,
	const VkImageUsageFlags usage,
	const VkImageAspectFlags aspect,
	const uint32_t layers = 1,
	const bool array_view = false
) {
	const VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = {extent.width, extent.height, 1},
		.mipLevels = mip_levels,
		.arrayLayers = layers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	const VmaAllocationCreateInfo allocation_info = {.usage = VMA_MEMORY_USAGE_AUTO};

	Image image;
	image.format = format;
	image.extent = extent;
	image.mip_levels = mip_levels;
	image.layers = layers;
	if (vmaCreateImage(vk_context.allocator, &image_info, &allocation_info, &image.image, &image.allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error(std::format("Allocating a {}x{} Vulkan image failed", extent.width, extent.height));
	}

	const VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.image,
		.viewType = (array_view || layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {aspect, 0, mip_levels, 0, layers},
	};
	vkCreateImageView(vk_context.device.device, &view_info, nullptr, &image.view);
	return image;
}

export void destroy_image_deferred(const Image& image) {
	if (image.image == VK_NULL_HANDLE) {
		return;
	}
	vk_context.defer_destroy([image = image.image, view = image.view, allocation = image.allocation] {
		vkDestroyImageView(vk_context.device.device, view, nullptr);
		vmaDestroyImage(vk_context.allocator, image, allocation);
	});
}

export void image_barrier(
	const VkCommandBuffer cmd,
	const VkImage image,
	const VkImageLayout old_layout,
	const VkImageLayout new_layout,
	const VkPipelineStageFlags2 src_stage,
	const VkAccessFlags2 src_access,
	const VkPipelineStageFlags2 dst_stage,
	const VkAccessFlags2 dst_access,
	const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
	const uint32_t base_mip = 0,
	const uint32_t mip_count = VK_REMAINING_MIP_LEVELS
) {
	const VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage,
		.srcAccessMask = src_access,
		.dstStageMask = dst_stage,
		.dstAccessMask = dst_access,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {aspect, base_mip, mip_count, 0, VK_REMAINING_ARRAY_LAYERS},
	};
	const VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};
	vkCmdPipelineBarrier2(cmd, &dependency);
}

/// Bump allocator over persistently mapped host-visible memory, for data written once per frame
/// (bone matrices, particle vertices, overlay pixels). One per frame in flight; reset() once that frame's fence has signaled.
export class FrameAllocator {
	static constexpr VkDeviceSize minimum_chunk_size = 4 * 1024 * 1024;
	static constexpr VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	std::vector<Buffer> chunks;
	VkDeviceSize offset = 0;

  public:
	struct Allocation {
		VkBuffer buffer;
		VkDeviceSize offset;
		std::byte* data;
		VkDeviceAddress address;
	};

	FrameAllocator() = default;
	FrameAllocator(const FrameAllocator&) = delete;
	FrameAllocator& operator=(const FrameAllocator&) = delete;

	~FrameAllocator() {
		for (const auto& chunk : chunks) {
			destroy_buffer_deferred(chunk);
		}
	}

	Allocation allocate(const VkDeviceSize size, const VkDeviceSize alignment = 256) {
		VkDeviceSize aligned = (offset + alignment - 1) / alignment * alignment;
		if (chunks.empty() || aligned + size > chunks.back().size) {
			chunks.push_back(create_buffer(std::max(minimum_chunk_size, size), usage, true));
			aligned = 0;
		}
		const Buffer& chunk = chunks.back();
		offset = aligned + size;
		return {
			.buffer = chunk.buffer,
			.offset = aligned,
			.data = static_cast<std::byte*>(chunk.mapped) + aligned,
			.address = chunk.address + aligned,
		};
	}

	template <typename T>
	Allocation upload(const std::span<const T> data, const VkDeviceSize alignment = 256) {
		const Allocation allocation = allocate(data.size_bytes(), alignment);
		std::memcpy(allocation.data, data.data(), data.size_bytes());
		return allocation;
	}

	/// Only call once the GPU is done with everything allocated since the last reset
	void reset() {
		// A frame that overflowed into several chunks gets a single chunk big enough for all of it next time
		if (chunks.size() > 1) {
			VkDeviceSize total = 0;
			for (const auto& chunk : chunks) {
				total += chunk.size;
				destroy_buffer_deferred(chunk);
			}
			chunks.clear();
			chunks.push_back(create_buffer(total, usage, true));
		}
		offset = 0;
	}
};

/// The global bindless texture table: set 0, binding 0 is `sampler2D textures[]`, indexed by slot.
/// Every pipeline uses the same layout: this set plus a 128 byte push constant range visible to all stages.
export class Bindless {
	std::mutex mutex;
	std::vector<uint32_t> free_slots;
	uint32_t next_slot = 0;

  public:
	static constexpr uint32_t push_constant_size = 128;

	VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet set = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	uint32_t capacity = 0;

	/// Linear filtering with mipmaps; index is (repeat_u ? 1 : 0) | (repeat_v ? 2 : 0), matching MDX texture flags
	std::array<VkSampler, 4> samplers {};

	/// Nearest filtering, clamped. Required for integer formats, which can't be filtered linearly.
	VkSampler nearest_sampler = VK_NULL_HANDLE;

	void init() {
		const VkDevice device = vk_context.device.device;

		VkPhysicalDeviceVulkan12Properties properties_12 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
		VkPhysicalDeviceProperties2 properties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &properties_12,
		};
		vkGetPhysicalDeviceProperties2(vk_context.physical_device.physical_device, &properties);
		capacity = std::min({
			16u * 1024u,
			properties_12.maxDescriptorSetUpdateAfterBindSampledImages,
			properties_12.maxDescriptorSetUpdateAfterBindSamplers,
			properties_12.maxPerStageDescriptorUpdateAfterBindSampledImages,
			properties_12.maxPerStageDescriptorUpdateAfterBindSamplers,
		});

		const VkDescriptorSetLayoutBinding binding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = capacity,
			.stageFlags = VK_SHADER_STAGE_ALL,
		};
		const VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
			| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
		const VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = 1,
			.pBindingFlags = &binding_flags,
		};
		const VkDescriptorSetLayoutCreateInfo layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &flags_info,
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = 1,
			.pBindings = &binding,
		};
		vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &set_layout);

		const VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, capacity};
		const VkDescriptorPoolCreateInfo pool_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = &pool_size,
		};
		vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);

		const VkDescriptorSetAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &set_layout,
		};
		vkAllocateDescriptorSets(device, &allocate_info, &set);

		const VkPushConstantRange push_range = {VK_SHADER_STAGE_ALL, 0, push_constant_size};
		const VkPipelineLayoutCreateInfo pipeline_layout_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &set_layout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &push_range,
		};
		vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout);

		for (uint32_t i = 0; i < samplers.size(); i++) {
			const VkSamplerCreateInfo sampler_info = {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = (i & 1) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeV = (i & 2) ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.maxLod = VK_LOD_CLAMP_NONE,
			};
			vkCreateSampler(device, &sampler_info, nullptr, &samplers[i]);
		}

		const VkSamplerCreateInfo nearest_info = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxLod = VK_LOD_CLAMP_NONE,
		};
		vkCreateSampler(device, &nearest_info, nullptr, &nearest_sampler);
	}

	void destroy() {
		const VkDevice device = vk_context.device.device;
		for (auto& sampler : samplers) {
			vkDestroySampler(device, sampler, nullptr);
		}
		vkDestroySampler(device, nearest_sampler, nullptr);
		vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		vkDestroyDescriptorPool(device, pool, nullptr);
		vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
	}

	/// Writes a combined image sampler into a free slot and returns the slot. Thread-safe.
	/// The image must be in SHADER_READ_ONLY_OPTIMAL layout whenever a shader reads the slot.
	uint32_t add(const VkImageView view, const VkSampler sampler) {
		std::lock_guard lock(mutex);

		uint32_t slot;
		if (!free_slots.empty()) {
			slot = free_slots.back();
			free_slots.pop_back();
		} else if (next_slot < capacity) {
			slot = next_slot++;
		} else {
			throw std::runtime_error(std::format("The bindless texture table is full ({} textures)", capacity));
		}

		const VkDescriptorImageInfo image_info = {
			.sampler = sampler,
			.imageView = view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		const VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = set,
			.dstBinding = 0,
			.dstArrayElement = slot,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &image_info,
		};
		vkUpdateDescriptorSets(vk_context.device.device, 1, &write, 0, nullptr);
		return slot;
	}

	/// Returns the slot to the free list once the GPU can no longer be reading it. Thread-safe.
	void remove(const uint32_t slot) {
		vk_context.defer_destroy([this, slot] {
			std::lock_guard lock(mutex);
			free_slots.push_back(slot);
		});
	}

	void bind(const VkCommandBuffer cmd) const {
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &set, 0, nullptr);
	}
};

/// Never destroyed, for the same reason as vk_context
export inline Bindless& bindless = *new Bindless;

std::vector<uint32_t> load_spirv(const fs::path& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw std::runtime_error(std::format("Missing SPIR-V shader {}", path.string()));
	}
	std::vector<uint32_t> code(static_cast<size_t>(file.tellg()) / sizeof(uint32_t));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(code.size() * sizeof(uint32_t)));
	return code;
}

/// Every viewport renders to a swapchain image in this format plus a depth attachment in depth_attachment_format.
/// Desktop drivers and MoltenVK all offer both, so pipelines are built once, for these formats.
export constexpr VkFormat color_attachment_format = VK_FORMAT_B8G8R8A8_UNORM;
export constexpr VkFormat depth_attachment_format = VK_FORMAT_D32_SFLOAT;

/// Everything that varies between HiveWE's graphics pipelines. The rest is fixed: the bindless pipeline layout,
/// counter-clockwise front faces (matching OpenGL under the flipped viewport), no MSAA, depth compare LEQUAL,
/// no vertex input (shaders pull vertices through buffer device addresses),
/// and dynamic viewport, scissor, cull mode, depth test, depth write and polygon mode.
export struct PipelineDescription {
	fs::path vertex_shader;
	fs::path fragment_shader;
	VkFormat color_format = color_attachment_format;
	VkFormat depth_format = depth_attachment_format;
	bool blend = false;
	VkBlendFactor src_factor = VK_BLEND_FACTOR_ONE;
	VkBlendFactor dst_factor = VK_BLEND_FACTOR_ZERO;
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

export VkPipeline create_graphics_pipeline(const PipelineDescription& description) {
	const VkDevice device = vk_context.device.device;

	// maintenance5 lets the SPIR-V be passed straight to the pipeline instead of through a VkShaderModule
	const std::vector<uint32_t> vertex = load_spirv(description.vertex_shader);
	const std::vector<uint32_t> fragment = load_spirv(description.fragment_shader);
	const std::array modules = {
		VkShaderModuleCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = vertex.size() * sizeof(uint32_t),
			.pCode = vertex.data(),
		},
		VkShaderModuleCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = fragment.size() * sizeof(uint32_t),
			.pCode = fragment.data(),
		},
	};
	const std::array stages = {
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = &modules[0],
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.pName = "main",
		},
		VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = &modules[1],
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pName = "main",
		},
	};

	// Vertex shaders read their vertices through buffer device addresses
	const VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};
	const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = description.topology,
	};
	const VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	const VkPipelineRasterizationStateCreateInfo rasterization = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.f,
	};
	const VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	const VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	};
	// MDX blend modes use the same factors for color and alpha
	const VkPipelineColorBlendAttachmentState blend_attachment = {
		.blendEnable = description.blend,
		.srcColorBlendFactor = description.src_factor,
		.dstColorBlendFactor = description.dst_factor,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = description.src_factor,
		.dstAlphaBlendFactor = description.dst_factor,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	const VkPipelineColorBlendStateCreateInfo blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blend_attachment,
	};
	constexpr std::array dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_CULL_MODE,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
		VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
		VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
	};
	const VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
		.pDynamicStates = dynamic_states.data(),
	};
	const VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &description.color_format,
		.depthAttachmentFormat = description.depth_format,
	};
	const VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &rendering,
		.stageCount = static_cast<uint32_t>(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterization,
		.pMultisampleState = &multisample,
		.pDepthStencilState = description.depth_format != VK_FORMAT_UNDEFINED ? &depth_stencil : nullptr,
		.pColorBlendState = &blend,
		.pDynamicState = &dynamic,
		.layout = bindless.pipeline_layout,
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	const VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(std::format("Creating the pipeline for {} failed ({})", description.vertex_shader.string(), static_cast<int>(result)));
	}
	return pipeline;
}

/// Owns a graphics pipeline and destroys it once the GPU can no longer be using it
export class Pipeline {
	VkPipeline pipeline = VK_NULL_HANDLE;

	void reset() {
		if (pipeline != VK_NULL_HANDLE && vk_context.is_initialized()) {
			vk_context.defer_destroy([pipeline = pipeline] {
				vkDestroyPipeline(vk_context.device.device, pipeline, nullptr);
			});
		}
		pipeline = VK_NULL_HANDLE;
	}

  public:
	Pipeline() = default;
	explicit Pipeline(const PipelineDescription& description) : pipeline(create_graphics_pipeline(description)) {}
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;
	Pipeline(Pipeline&& other) noexcept : pipeline(std::exchange(other.pipeline, VK_NULL_HANDLE)) {}
	Pipeline& operator=(Pipeline&& other) noexcept {
		if (this != &other) {
			reset();
			pipeline = std::exchange(other.pipeline, VK_NULL_HANDLE);
		}
		return *this;
	}
	~Pipeline() {
		reset();
	}

	operator VkPipeline() const {
		return pipeline;
	}
};

/// Sets the viewport to a rectangle given in framebuffer pixels from the top-left, with a negative viewport height
/// so clip space is Y-up like OpenGL. The rectangle may extend past the framebuffer; the scissor is clipped to it.
/// Returns that scissor, which is empty when nothing of the rectangle is on screen.
export VkRect2D set_viewport(const VkCommandBuffer cmd, const VkRect2D rect, const VkExtent2D framebuffer) {
	const VkViewport viewport = {
		static_cast<float>(rect.offset.x),
		static_cast<float>(rect.offset.y + static_cast<int32_t>(rect.extent.height)),
		static_cast<float>(rect.extent.width),
		-static_cast<float>(rect.extent.height),
		0.f,
		1.f,
	};
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	const int64_t left = std::max<int64_t>(rect.offset.x, 0);
	const int64_t top = std::max<int64_t>(rect.offset.y, 0);
	const int64_t right = std::min<int64_t>(int64_t {rect.offset.x} + rect.extent.width, framebuffer.width);
	const int64_t bottom = std::min<int64_t>(int64_t {rect.offset.y} + rect.extent.height, framebuffer.height);
	const VkRect2D scissor = {
		{static_cast<int32_t>(left), static_cast<int32_t>(top)},
		{static_cast<uint32_t>(std::max<int64_t>(right - left, 0)), static_cast<uint32_t>(std::max<int64_t>(bottom - top, 0))},
	};
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	return scissor;
}
