#include "GPUTexture.h"

#include <QOpenGLFunctions_4_5_Core>
#include <SOIL2/SOIL2.h>

#include "BLP.h"
#include "Hierarchy.h"

GPUTexture::GPUTexture(const fs::path& path) {
	/*gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
	gl->glTextureStorage2D(id, 1, GL_RGBA8, 1, 1);
	return;*/
	try {
		BinaryReader reader = hierarchy.open_file(path);

		if (path.extension() == ".blp" || path.extension() == ".BLP") {
			blp::BLP blp = blp::BLP(reader);

			gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
			gl->glTextureStorage2D(id, log2(std::max(blp.width, blp.height)) + 1, GL_RGBA8, blp.width, blp.height);
			gl->glTextureSubImage2D(id, 0, 0, 0, blp.width, blp.height, GL_BGRA, GL_UNSIGNED_BYTE, blp.data.data());
			gl->glGenerateTextureMipmap(id);
		} else {
			id = SOIL_load_OGL_texture_from_memory(reader.buffer.data(), reader.buffer.size(), SOIL_LOAD_AUTO, SOIL_LOAD_AUTO, SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT);
		}

		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} catch (const std::exception & e) {
		std::cout << "Error while loading texture " << path << " With error " << e.what() << "\n";

		BinaryReader reader = hierarchy.open_file("Textures/btntempw.dds");
		id = SOIL_load_OGL_texture_from_memory(reader.buffer.data(), reader.buffer.size(), SOIL_LOAD_AUTO, SOIL_LOAD_AUTO, SOIL_FLAG_MIPMAPS);
	};
}