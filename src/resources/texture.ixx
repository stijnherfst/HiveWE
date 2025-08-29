module;

#include <vector>
#include <filesystem>

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

		uint8_t* image_data;

		if (new_path.extension() == ".blp") {
			image_data = blp::load(reader, width, height, channels);
		} else {
			image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
		delete image_data;
	}
};