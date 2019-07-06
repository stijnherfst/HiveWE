#include "stdafx.h"

GroundTexture::GroundTexture(const fs::path& path) {
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		BinaryReader reader = hierarchy.open_file(path);

		blp::BLP blp = blp::BLP(reader);
		auto[width, height, data] = blp.mipmaps.front();

		tile_size = height * 0.25;
		extended = (width == height * 2);
		
		gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
		gl->glTextureStorage3D(id, log2(tile_size) + 1, GL_RGBA8, tile_size, tile_size, extended ? 32 : 16);
		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, data.data() + (y * tile_size * width + x * tile_size) * 4);

				if (extended) {
					gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, data.data() + (y * tile_size * width + (x + 4) * tile_size) * 4);
				}
			}
		}
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		gl->glGenerateTextureMipmap(id);

		// Find mipmap where each tile is 1x1 px
		int mipmap_number = std::max(0.0, std::log2(width) - 3.0);
		auto[mipw, miph, mipmap] = blp.mipmaps[mipmap_number];
		minimap_color = *reinterpret_cast<glm::u8vec4*>(mipmap.data());
		std::swap(minimap_color.r, minimap_color.b);
	} else {
		static_assert("Haven't handled loading non .blp images yet");
		id = SOIL_load_OGL_texture(path.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	}
}