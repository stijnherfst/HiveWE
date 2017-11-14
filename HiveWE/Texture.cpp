#include "stdafx.h"

Texture::Texture(const std::string& path) {
	if (fs::path(path).extension() == ".blp") {
		auto [texture_data, w, h] = blp::BLP::load(path);
		data = texture_data;
		width = w;
		height = h;
	} else {
		data = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	}
}