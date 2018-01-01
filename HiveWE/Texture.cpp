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

GPUTexture::GPUTexture(const std::string& path) {
	if (fs::path(path).extension() == ".blp") {
		auto[data, width, height] = blp::BLP::load(path);

		gl->glCreateTextures(GL_TEXTURE_2D, 1, &id);
		gl->glTextureStorage2D(id, std::log(std::max(width, height)) + 1, GL_RGBA8, width, height);
		gl->glTextureSubImage2D(id, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gl->glGenerateTextureMipmap(id);
	} else {
		id = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	}
}