#include "stdafx.h"

ResourceManager resource_manager;

void Texture::load(const std::string& path) {
	data = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
}

void Texture::unload() {

}