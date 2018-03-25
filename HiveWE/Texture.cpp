#include "stdafx.h"

Texture::Texture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		auto [texture_data, w, h] = blp::BLP::load(path);
		data = texture_data;
		width = w;
		height = h;
		channels = 4;
	} else {
		data = SOIL_load_image(path.string().c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	}
}