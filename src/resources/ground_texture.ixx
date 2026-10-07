module;

#include <volk.h>

export module GroundTexture;

import std;
import BinaryReader;
import ResourceManager;
import BLP;
import Hierarchy;
import VkContext;
import VkResources;
import VkTexture;
import <soil2/SOIL2.h>;
import <glm/glm.hpp>;

namespace fs = std::filesystem;

/// A tileset ground texture: its 4x4 (or 8x4 when extended) grid of tiles as layers of a 2D array texture
export class GroundTexture : public Resource {
  public:
	Image image;
	/// Bindless slot of the array texture
	uint32_t slot = 0;
	int tile_size;
	bool extended = false;
	glm::vec4 minimap_color;

	static constexpr const char* name = "GroundTexture";

	explicit GroundTexture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.remastered) {
			new_path.replace_filename(path.stem().string() + "_diffuse");
		}

		BinaryReader reader = hierarchy.open_file(new_path, Hierarchy::FileSource::all, { ".dds", ".blp", ".tga" })
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", new_path.string());
				return hierarchy.open_file("Textures/btntempw.dds");
			})
			.value();

		int width = 0;
		int height = 0;
		int channels = 0;

		std::vector<uint8_t> blp_data;
		uint8_t* soil_data = nullptr;
		// To avoid intermediate memcpy
		const uint8_t* pixels = nullptr;

		if (blp::is_blp(reader)) {
			auto blp_image = blp::load(reader);
			if (!blp_image) {
				throw std::runtime_error(std::format("Failed to load ground texture {}: {}", new_path.string(), blp_image.error()));
			}
			width = blp_image->width;
			height = blp_image->height;
			channels = blp_image->channels;
			blp_data = std::move(blp_image->data);
			pixels = blp_data.data();
		} else {
			soil_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (soil_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode ground texture {}", new_path.string()));
			}
			pixels = soil_data;
		}

		tile_size = std::max(height * 0.25f, 1.f);
		extended = (width == height * 2);
		const int layers = extended ? 32 : 16;

		// Cut the tile grid into layers, left to right then top to bottom, with the extended half after the first 16
		const size_t tile_bytes = static_cast<size_t>(tile_size) * tile_size * 4;
		std::vector<uint8_t> tiles(tile_bytes * layers);
		for (int layer = 0; layer < layers; layer++) {
			const int tile_x = (layer % 16) % 4 + (layer >= 16 ? 4 : 0);
			const int tile_y = (layer % 16) / 4;
			for (int y = 0; y < tile_size; y++) {
				for (int x = 0; x < tile_size; x++) {
					const uint8_t* source = pixels + ((tile_y * tile_size + y) * width + tile_x * tile_size + x) * channels;
					uint8_t* target = tiles.data() + layer * tile_bytes + (static_cast<size_t>(y) * tile_size + x) * 4;
					for (int c = 0; c < 4; c++) {
						target[c] = c < channels ? source[c] : (c == 3 ? 255 : 0);
					}
				}
			}
		}
		free(soil_data);

		// The average color of the first tile, what its smallest mip level would hold
		glm::dvec4 sum(0.0);
		for (size_t i = 0; i < tile_bytes; i += 4) {
			sum += glm::dvec4(tiles[i], tiles[i + 1], tiles[i + 2], tiles[i + 3]);
		}
		minimap_color = glm::vec4(sum / static_cast<double>(tile_bytes / 4));

		image = create_rgba8_array_image(
			{static_cast<uint32_t>(tile_size), static_cast<uint32_t>(tile_size)},
			static_cast<uint32_t>(layers),
			tiles
		);
		slot = bindless.add(image.view, bindless.samplers[0]);
	}

	~GroundTexture() override {
		if (!vk_context.is_initialized()) {
			return;
		}
		bindless.remove(slot);
		destroy_image_deferred(image);
	}
};
