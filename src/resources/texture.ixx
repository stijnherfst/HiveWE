module;

#include <vector>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <utility>

export module Texture;

import std;
import Hierarchy;
import BLP;
import BinaryReader;
import ResourceManager;
import <soil2/SOIL2.h>;

namespace fs = std::filesystem;

export fs::path diffuse_texture_path(const fs::path& path) {
	if (!hierarchy.remastered) {
		return path;
	}
	fs::path new_path = path;
	new_path.replace_filename(path.stem().string() + "_diffuse");
	return new_path;
}

export class Texture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	explicit Texture(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, { ".dds", ".blp", ".tga" })
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", path.string());
				return hierarchy.open_file("Textures/btntempw.dds");
			})
			.value();

		if (blp::is_blp(reader)) {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load texture {}: {}", path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			data = std::move(image->data);
		} else {
			uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (image_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode texture {}", path.string()));
			}
			data = std::vector<uint8_t>(image_data, image_data + static_cast<size_t>(width) * height * channels);
			free(image_data);
		}
	}
};