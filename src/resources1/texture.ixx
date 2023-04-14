module;

#include <vector>
#include <filesystem>
#include <soil2/SOIL2.h>

export module Texture;

namespace fs = std::filesystem;

import Hierarchy;
import BLP;
import BinaryReader;
import ResourceManager;

export class Texture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	Texture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.hd) {
			new_path.replace_filename(path.stem().string() + "_diffuse.dds");
		}
		if (!hierarchy.file_exists(new_path)) {
			new_path = path;
			new_path.replace_extension(".blp");
			if (!hierarchy.file_exists(new_path)) {
				new_path.replace_extension(".dds");
			}
		}

		BinaryReader reader = hierarchy.open_file(new_path);
		uint8_t* image_data;

		if (new_path.extension() == ".blp" || new_path.extension() == ".BLP") {
			image_data = blp::load(reader, width, height, channels);
		} else {
			image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
		delete image_data;
	}
};