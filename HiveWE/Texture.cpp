#include "stdafx.h"

Texture::Texture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		BinaryReader reader = hierarchy.open_file(path);
		auto[w, h, d] = blp::BLP(reader).mipmaps.front();

		data = d;
		width = w;
		height = h;
		channels = 4;
	} else {
		uint8_t* image_data = SOIL_load_image(path.string().c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
	}
}