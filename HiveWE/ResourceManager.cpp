#include "stdafx.h"

ResourceManager resource_manager;

void Texture::load(const std::string& path) {
	if (fs::path(path).extension() == ".blp") {
		data = blp::BLP::load(path);
		width = 512;
		height = 256;
	} else {
		data = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	}
}

void Texture::unload() {
	delete[] data;
}