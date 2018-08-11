#include "stdafx.h"

GPUTexture::GPUTexture(const fs::path& path) {
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &id);
	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		BinaryReader reader = hierarchy.open_file(path);
		if (reader.buffer.size()) {
			blp::BLP blp = blp::BLP(reader);

			gl->glTextureStorage2D(id, log2(std::max(blp.width, blp.height)) + 1, GL_RGBA8, blp.width, blp.height);

			int mipmap = 0;
			for (auto&&[width, height, data] : blp.mipmaps) {
				gl->glTextureSubImage2D(id, mipmap++, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data.data());
			}

			//if (blp.mipmaps.size() == 1) {
			//	gl->glGenerateTextureMipmap(id);
			//}

			gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			gl->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	} else {
		id = SOIL_load_OGL_texture(path.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	}
}