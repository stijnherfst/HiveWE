#include "GPUTexture.h"

#include <QOpenGLFunctions_4_5_Core>
#include <SOIL2/SOIL2.h>

#include "BLP.h"
#include "Hierarchy.h"

GPUTexture::GPUTexture(const fs::path& path) {
	fs::path new_path = path;

	if (hierarchy.hd) {
		new_path.replace_filename(path.stem().string() + "_diffuse.dds");
	}
	if (!hierarchy.file_exists(new_path)) {
		new_path = path;
		new_path.replace_extension(".blp");
		if (!hierarchy.file_exists(new_path)) {
			new_path.replace_extension(".dds");
		}
	}

	// This exception checking can be removed later on since only non existing replaceabletextures cause this to crash (reforged beta issue)
	try {
		BinaryReader reader = hierarchy.open_file(new_path);

		if (new_path.extension() == ".blp") {
			int width;
			int height;
			int channels;
			uint8_t* data = blp::load(reader, width, height, channels);

			gl->glCreateTextures(GL_TEXTURE_2D, 1, &id);
			gl->glTextureStorage2D(id, log2(std::max(width, height)) + 1, GL_RGBA8, width, height);
			gl->glTextureSubImage2D(id, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
			gl->glGenerateTextureMipmap(id);
			delete data;
		} else {
			id = SOIL_load_OGL_texture_from_memory(reader.buffer.data(), reader.buffer.size(), SOIL_LOAD_AUTO, SOIL_LOAD_AUTO, SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT);
		}

		gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} catch (const std::exception & e) {
		std::cout << "Error while loading texture " << new_path << " With error " << e.what() << "\n";

		BinaryReader reader = hierarchy.open_file("Textures/btntempw.dds");
		id = SOIL_load_OGL_texture_from_memory(reader.buffer.data(), reader.buffer.size(), SOIL_LOAD_AUTO, SOIL_LOAD_AUTO, SOIL_FLAG_MIPMAPS);
	};
}