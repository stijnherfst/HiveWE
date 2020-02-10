#include "Texture.h"

#include <SOIL2/SOIL2.h>

#include "BinaryReader.h"
#include "Hierarchy.h"
#include "BLP.h"

Texture::Texture(const fs::path& path) {
	BinaryReader reader = hierarchy.open_file(path);
	uint8_t* image_data;

	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		image_data = blp::load(reader, width, height, channels);
	} else {
		image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
	}
	data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
	delete image_data;
}