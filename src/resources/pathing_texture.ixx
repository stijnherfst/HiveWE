export module PathingTexture;

import std;
import types;
import ResourceManager;
import Hierarchy;
import BLP;
import BinaryReader;
import <soil2/SOIL2.h>;
import <glm/glm.hpp>;

namespace fs = std::filesystem;

export class PathingTexture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<u8> data;

	bool homogeneous;

	static constexpr const char* name = "PathingTexture";

	explicit PathingTexture(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path).value();

		if (path.extension() == ".blp" || path.extension() == ".BLP") {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load pathing texture {}: {}", path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			data = std::move(image->data);
		} else {
			uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (image_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode pathing texture {}", path.string()));
			}
			data = std::vector<u8>(image_data, image_data + static_cast<size_t>(width) * height * channels);
			free(image_data);
		}

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