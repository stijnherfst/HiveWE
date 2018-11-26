#include "stdafx.h"

Texture::Texture(const fs::path& path) {
	BinaryReader reader = hierarchy.open_file(path);
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		auto blp = blp::BLP(reader);
		auto[w, h, d] = blp.mipmaps.front();

		data = d;
		width = w;
		height = h;
		channels = 4;

		int mipmap_number = std::log2(width) - 2;
		auto[mipw, miph, mipmap] = blp.mipmaps[mipmap_number];
		clr = *reinterpret_cast<glm::u8vec4*>(mipmap.data());
		std::swap(clr.r, clr.b);
	} else {
		uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), reader.buffer.size(), &width, &height, &channels, SOIL_LOAD_AUTO);
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
	}
}