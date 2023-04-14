module;

#include <vector>
#include <filesystem>
#include <soil2/SOIL2.h>
#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

export module PathingTexture;

namespace fs = std::filesystem;

import ResourceManager;
import Hierarchy;
import BLP;
import BinaryReader;

export class PathingTexture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	bool homogeneous;

	static constexpr const char* name = "PathingTexture";

	explicit PathingTexture(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path);
		uint8_t* image_data;

		if (path.extension() == ".blp" || path.extension() == ".BLP") {
			image_data = blp::load(reader, width, height, channels);
		} else {
			image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
		delete image_data;

		homogeneous = true;
		for (size_t i = 0; i < data.size(); i += channels) {
			if (channels == 3) {
				homogeneous = homogeneous && *reinterpret_cast<glm::u8vec3*>(data.data() + i) == *reinterpret_cast<glm::u8vec3*>(data.data());
			} else if (channels == 4) {
				homogeneous = homogeneous && *reinterpret_cast<glm::u8vec4*>(data.data() + i) == *reinterpret_cast<glm::u8vec4*>(data.data());
			}
		}
	}
};