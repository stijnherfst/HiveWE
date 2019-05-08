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

		// ToDo does this really belong here? Only a few textures have a minimap color
		int mipmap_number = std::log2(width) - 2;
		if (mipmap_number < blp.mipmaps.size() - 1) {
			auto[mipw, miph, mipmap] = blp.mipmaps[mipmap_number];
			minimap_color = *reinterpret_cast<glm::u8vec4*>(mipmap.data());
			std::swap(minimap_color.r, minimap_color.b);
		}
	} else {
		uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
		delete image_data;
	}
}