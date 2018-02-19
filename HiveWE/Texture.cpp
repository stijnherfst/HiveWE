#include "stdafx.h"

Texture::Texture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		auto [texture_data, w, h] = blp::BLP::load(path);
		data = texture_data;
		width = w;
		height = h;
	} else {
		data = SOIL_load_image(path.string().c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
	}
}

GPUTexture::GPUTexture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		auto[data, width, height] = blp::BLP::load(path); // ToDo use resource manager here too?

		gl->glCreateTextures(GL_TEXTURE_2D, 1, &id);
		gl->glTextureStorage2D(id, std::log(std::max(width, height)) + 1, GL_RGBA8, width, height);
		gl->glTextureSubImage2D(id, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gl->glGenerateTextureMipmap(id);

		delete data;
	} else {
		id = SOIL_load_OGL_texture(path.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	}
}