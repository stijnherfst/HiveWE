module;

#include <volk.h>
#include <vk_mem_alloc.h>

export module VkTexture;

import std;
import types;
import BinaryReader;
import ResourceManager;
import Hierarchy;
import BLP;
import VkContext;
import VkResources;
import <soil2/SOIL2.h>;

namespace fs = std::filesystem;

namespace {
	struct TextureData {
		VkFormat format;
		VkExtent2D extent;
		/// Byte ranges of each mip level inside `pixels`. A single level is expanded into a full chain on the GPU.
		std::vector<std::pair<size_t, size_t>> levels;
		std::vector<u8> pixels;
		/// Array layers, packed one after another. Only supported for single-level data.
		uint32_t layers = 1;
		bool array_view = false;
	};

	uint32_t read_u32(const std::span<const u8> data, const size_t offset) {
		uint32_t value;
		std::memcpy(&value, data.data() + offset, sizeof(value));
		return value;
	}

	constexpr uint32_t four_cc(const char a, const char b, const char c, const char d) {
		return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
	}

	/// Block-compressed DDS files are uploaded as-is with the mip levels stored in the file.
	/// Returns nothing for DDS variants that need decoding instead.
	std::optional<TextureData> parse_compressed_dds(const std::span<const u8> data) {
		constexpr size_t header_size = 128;
		constexpr size_t dx10_header_size = 20;
		constexpr uint32_t ddpf_fourcc = 0x4;
		constexpr uint32_t ddsd_mipmapcount = 0x20000;

		if (data.size() < header_size || read_u32(data, 0) != four_cc('D', 'D', 'S', ' ')) {
			return std::nullopt;
		}
		const uint32_t flags = read_u32(data, 8);
		const uint32_t height = read_u32(data, 12);
		const uint32_t width = read_u32(data, 16);
		const uint32_t stored_mips = (flags & ddsd_mipmapcount) ? std::max(1u, read_u32(data, 28)) : 1u;
		const uint32_t pixel_format_flags = read_u32(data, 80);
		const uint32_t fourcc = read_u32(data, 84);

		if (!(pixel_format_flags & ddpf_fourcc) || width == 0 || height == 0 || width > 16384 || height > 16384) {
			return std::nullopt;
		}

		size_t offset = header_size;
		VkFormat format;
		uint32_t block_size = 16;
		switch (fourcc) {
			case four_cc('D', 'X', 'T', '1'):
				format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
				block_size = 8;
				break;
			case four_cc('D', 'X', 'T', '3'):
				format = VK_FORMAT_BC2_UNORM_BLOCK;
				break;
			case four_cc('D', 'X', 'T', '5'):
				format = VK_FORMAT_BC3_UNORM_BLOCK;
				break;
			case four_cc('A', 'T', 'I', '2'):
				format = VK_FORMAT_BC5_UNORM_BLOCK;
				break;
			case four_cc('D', 'X', '1', '0'): {
				if (data.size() < header_size + dx10_header_size) {
					return std::nullopt;
				}
				offset += dx10_header_size;
				switch (read_u32(data, header_size)) { // DXGI_FORMAT
					case 71:
						format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
						block_size = 8;
						break;
					case 74:
						format = VK_FORMAT_BC2_UNORM_BLOCK;
						break;
					case 77:
						format = VK_FORMAT_BC3_UNORM_BLOCK;
						break;
					case 80:
						format = VK_FORMAT_BC4_UNORM_BLOCK;
						block_size = 8;
						break;
					case 83:
						format = VK_FORMAT_BC5_UNORM_BLOCK;
						break;
					case 98:
						format = VK_FORMAT_BC7_UNORM_BLOCK;
						break;
					default:
						return std::nullopt;
				}
				break;
			}
			default:
				return std::nullopt;
		}

		TextureData texture;
		texture.format = format;
		texture.extent = {width, height};

		// Level offsets are relative to the start of the pixel data, keeping them multiples of the block size as copies require.
		// Truncated files keep the levels that are fully present.
		const size_t pixels_start = offset;
		const uint32_t max_mips = static_cast<uint32_t>(std::bit_width(std::max(width, height)));
		uint32_t level_width = width;
		uint32_t level_height = height;
		for (uint32_t level = 0; level < std::min(stored_mips, max_mips); level++) {
			const size_t size = static_cast<size_t>((level_width + 3) / 4) * ((level_height + 3) / 4) * block_size;
			if (offset + size > data.size()) {
				break;
			}
			texture.levels.emplace_back(offset - pixels_start, size);
			offset += size;
			level_width = std::max(1u, level_width / 2);
			level_height = std::max(1u, level_height / 2);
		}
		if (texture.levels.empty()) {
			return std::nullopt;
		}

		texture.pixels.assign(data.begin() + pixels_start, data.begin() + offset);
		return texture;
	}

	std::expected<TextureData, std::string> decode(const fs::path& path, BinaryReader& reader) {
		if (blp::is_blp(reader)) {
			auto image = blp::load(reader);
			if (!image) {
				return std::unexpected(image.error());
			}
			TextureData texture;
			texture.format = VK_FORMAT_R8G8B8A8_UNORM;
			texture.extent = {static_cast<uint32_t>(image->width), static_cast<uint32_t>(image->height)};
			texture.levels.emplace_back(0, image->data.size());
			texture.pixels = std::move(image->data);
			return texture;
		}

		const std::span<const u8> bytes(reader.buffer.data(), reader.buffer.size());
		if (auto dds = parse_compressed_dds(bytes)) {
			return std::move(*dds);
		}

		int width;
		int height;
		int channels;
		u8* pixels = SOIL_load_image_from_memory(
			bytes.data(),
			static_cast<int>(bytes.size()),
			&width,
			&height,
			&channels,
			SOIL_LOAD_RGBA
		);
		if (!pixels) {
			return std::unexpected(std::format("{}: {}", path.string(), SOIL_last_result()));
		}
		TextureData texture;
		// Decoded (non-BLP, non-block-compressed) images are sampled as sRGB
		texture.format = VK_FORMAT_R8G8B8A8_SRGB;
		texture.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		const size_t size = static_cast<size_t>(width) * height * 4;
		texture.levels.emplace_back(0, size);
		texture.pixels.assign(pixels, pixels + size);
		std::free(pixels);
		return texture;
	}

	/// Stands in for a texture that failed to decode
	TextureData black_texture() {
		return {
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {1, 1},
			.levels = {{0, 4}},
			.pixels = {0, 0, 0, 255},
		};
	}
} // namespace

/// Uploads decoded or block-compressed texture data into a new sampled image in SHADER_READ_ONLY_OPTIMAL layout.
/// Blocks until the upload is done.
Image upload_texture(const TextureData& data, const bool generate_mips = true) {
	// Decoded images arrive as one level and get a full chain generated by blitting.
	// Block-compressed ones can't be blitted and keep the levels stored in the file.
	const bool block_compressed = data.format != VK_FORMAT_R8G8B8A8_UNORM && data.format != VK_FORMAT_R8G8B8A8_SRGB;
	const bool blit_mips = generate_mips && data.levels.size() == 1 && !block_compressed;
	const uint32_t mip_levels = blit_mips ? static_cast<uint32_t>(std::bit_width(std::max(data.extent.width, data.extent.height)))
										  : static_cast<uint32_t>(data.levels.size());

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (blit_mips) {
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	Image image = create_image(data.format, data.extent, mip_levels, usage, VK_IMAGE_ASPECT_COLOR_BIT, data.layers, data.array_view);

	Buffer staging = create_buffer(data.pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
	std::memcpy(staging.mapped, data.pixels.data(), data.pixels.size());

	vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
		image_barrier(
			cmd,
			image.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT
		);

		std::vector<VkBufferImageCopy> regions;
		for (uint32_t level = 0; level < data.levels.size(); level++) {
			regions.push_back({
				.bufferOffset = data.levels[level].first,
				.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, data.layers},
				.imageExtent = {std::max(1u, data.extent.width >> level), std::max(1u, data.extent.height >> level), 1},
			});
		}
		vkCmdCopyBufferToImage(
			cmd,
			staging.buffer,
			image.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()),
			regions.data()
		);

		if (blit_mips) {
			for (uint32_t level = 1; level < mip_levels; level++) {
				image_barrier(
					cmd,
					image.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_READ_BIT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					level - 1,
					1
				);
				const auto source_width = static_cast<int32_t>(std::max(1u, data.extent.width >> (level - 1)));
				const auto source_height = static_cast<int32_t>(std::max(1u, data.extent.height >> (level - 1)));
				const VkImageBlit blit = {
					.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, data.layers},
					.srcOffsets = {{0, 0, 0}, {source_width, source_height, 1}},
					.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, data.layers},
					.dstOffsets = {{0, 0, 0}, {std::max(1, source_width / 2), std::max(1, source_height / 2), 1}},
				};
				vkCmdBlitImage(
					cmd,
					image.image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&blit,
					VK_FILTER_LINEAR
				);
			}
			// Every level but the last was left as a blit source
			if (mip_levels > 1) {
				image_barrier(
					cmd,
					image.image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_READ_BIT,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					mip_levels - 1
				);
			}
			image_barrier(
				cmd,
				image.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				mip_levels - 1,
				1
			);
		} else {
			image_barrier(
				cmd,
				image.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
			);
		}
	});

	vmaDestroyBuffer(vk_context.allocator, staging.buffer, staging.allocation);
	return image;
}

/// Creates a 2D array image from layers of tightly packed 8-bit RGBA pixels, one after another, with a full mip chain.
/// Blocks until uploaded.
export Image create_rgba8_array_image(
	const VkExtent2D extent,
	const uint32_t layers,
	const std::span<const u8> pixels,
	const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM
) {
	TextureData data;
	data.format = format;
	data.extent = extent;
	data.levels.emplace_back(0, pixels.size());
	data.pixels.assign(pixels.begin(), pixels.end());
	data.layers = layers;
	data.array_view = true;
	return upload_texture(data);
}

/// Expands tightly packed 8-bit pixels with 1 to 4 channels to RGBA; missing color channels are 0 and missing alpha is 255
export std::vector<u8> expand_to_rgba8(const std::span<const u8> pixels, const int channels) {
	if (channels == 4) {
		return {pixels.begin(), pixels.end()};
	}
	const size_t count = pixels.size() / channels;
	std::vector<u8> rgba(count * 4, 0);
	for (size_t i = 0; i < count; i++) {
		for (int c = 0; c < channels; c++) {
			rgba[i * 4 + c] = pixels[i * channels + c];
		}
		rgba[i * 4 + 3] = 255;
	}
	return rgba;
}

/// A single-level 2D texture whose contents the CPU replaces while it is in use (pathing maps, brush shapes).
/// Registered in the bindless table; the slot changes when the size does.
export class UpdatableTexture {
	VkFormat format;
	VkSampler sampler;
	uint32_t bytes_per_pixel;

	void release() {
		if (image.image == VK_NULL_HANDLE || !vk_context.is_initialized()) {
			return;
		}
		bindless.remove(slot);
		destroy_image_deferred(image);
		image = {};
	}

  public:
	Image image;
	uint32_t slot = 0;

	UpdatableTexture(const VkFormat format, const VkSampler sampler, const uint32_t bytes_per_pixel)
		: format(format),
		  sampler(sampler),
		  bytes_per_pixel(bytes_per_pixel) {}

	UpdatableTexture(const UpdatableTexture&) = delete;
	UpdatableTexture& operator=(const UpdatableTexture&) = delete;

	~UpdatableTexture() {
		release();
	}

	/// Replaces the contents with tightly packed pixels. Waits for earlier GPU work reading the texture.
	void update(const VkExtent2D extent, const std::span<const u8> pixels) {
		if (image.extent.width != extent.width || image.extent.height != extent.height) {
			release();
			image = create_image(
				format,
				extent,
				1,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			slot = bindless.add(image.view, sampler);
		}

		const size_t size = static_cast<size_t>(extent.width) * extent.height * bytes_per_pixel;
		Buffer staging = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
		std::memcpy(staging.mapped, pixels.data(), std::min(size, pixels.size()));

		vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
			// UNDEFINED discards the old contents, which are fully overwritten; the stage mask still waits for earlier reads
			image_barrier(
				cmd,
				image.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT
			);
			const VkBufferImageCopy region = {
				.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				.imageExtent = {extent.width, extent.height, 1},
			};
			vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			image_barrier(
				cmd,
				image.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
			);
		});
		vmaDestroyBuffer(vk_context.allocator, staging.buffer, staging.allocation);
	}
};

/// Creates a sampled image from tightly packed 8-bit RGBA pixels, with a single mip level. Blocks until uploaded.
export Image create_rgba8_image(const VkExtent2D extent, const std::span<const u8> pixels, const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM) {
	TextureData data;
	data.format = format;
	data.extent = extent;
	data.levels.emplace_back(0, pixels.size());
	data.pixels.assign(pixels.begin(), pixels.end());
	// A single level would otherwise get a mip chain; UI images are drawn at their own size
	return upload_texture(data, false);
}

/// A sampled 2D texture registered in the bindless table.
/// flags are the MDX texture flags: bit 0 wraps U, bit 1 wraps V, otherwise clamps.
export class VulkanTexture : public Resource {
  public:
	Image image;
	uint32_t slot = 0;

	static constexpr const char* name = "VulkanTexture";

	explicit VulkanTexture(const fs::path& path, const int flags = 0) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, {".dds", ".blp", ".tga"})
								  .or_else([&](const std::string&) {
									  std::println("Error loading texture {}", path.string());
									  return hierarchy.open_file("Textures/btntempw.dds");
								  })
								  .value();

		TextureData data = decode(path, reader)
							   .or_else([&](const std::string& error) -> std::expected<TextureData, std::string> {
								   std::println("Error loading texture {}: {}", path.string(), error);
								   return black_texture();
							   })
							   .value();
		image = upload_texture(data);

		slot = bindless.add(image.view, bindless.samplers[flags & 3]);
	}

	~VulkanTexture() override {
		if (!vk_context.is_initialized()) {
			return;
		}
		bindless.remove(slot);
		destroy_image_deferred(image);
	}
};
