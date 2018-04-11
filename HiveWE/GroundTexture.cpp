#include "stdafx.h"

GroundTexture::GroundTexture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		auto[data, width, height] = blp::BLP::load(path); // ToDo use resource manager here too?
		tile_size = height * 0.25;
		extended = (width == height * 2);

		gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
		gl->glTextureStorage3D(id, std::log(tile_size) + 1, GL_RGBA8, tile_size, tile_size, extended ? 32 : 16);
		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + x * tile_size) * 4);

				if (extended) {
					gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + (x + 4) * tile_size) * 4);
				}
			}
		}
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		gl->glGenerateTextureMipmap(id);

		delete data;
	} else {
		static_assert("Haven't handled loading non .blp images yet");
		id = SOIL_load_OGL_texture(path.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	}
}