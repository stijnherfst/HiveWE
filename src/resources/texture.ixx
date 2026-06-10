module;

#include <vector>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <utility>

export module Texture;

import Hierarchy;
import BLP;
import BinaryReader;
import ResourceManager;
import <soil2/SOIL2.h>;

namespace fs = std::filesystem;

export class Texture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	explicit Texture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.hd) {
			new_path.replace_filename(path.stem().string() + "_diffuse");
		}

		new_path.replace_extension(".tga");
		BinaryReader reader = hierarchy.open_file(new_path)
			.or_else([&](const std::string&) {
				new_path.replace_extension(".blp");
				return hierarchy.open_file(new_path);
			})
			.or_else([&](const std::string&) {
				new_path.replace_extension(".dds");
				return hierarchy.open_file(new_path);
			})
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", new_path.string());
				new_path = "Textures/btntempw.dds";
				return hierarchy.open_file(new_path);
			})
			.value();

		if (new_path.extension() == ".blp") {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load texture {}: {}", new_path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			data = std::move(image->data);
		} else {
			uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (image_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode texture {}", new_path.string()));
			}
			data = std::vector<uint8_t>(image_data, image_data + static_cast<size_t>(width) * height * channels);
			free(image_data);
		}
	}
};