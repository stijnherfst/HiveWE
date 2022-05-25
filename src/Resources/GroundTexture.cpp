#include "GroundTexture.h"

#include <SOIL2/SOIL2.h>

import BLP;

import Hierarchy;


GroundTexture::GroundTexture(const fs::path& path) {
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
	
	BinaryReader reader = hierarchy.open_file(new_path);

	int width;
	int height;
	int channels;
	uint8_t* data;

	int upload_format = GL_RGBA;
	if (new_path.extension() == ".blp" || new_path.extension() == ".BLP") {
		data = blp::load(reader, width, height, channels);
		upload_format = GL_BGRA;
	} else {
		data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		upload_format = GL_RGBA;
	}

	tile_size = height * 0.25;
	extended = (width == height * 2);
	int lods = log2(tile_size) + 1;
		
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
	gl->glTextureStorage3D(id, lods, GL_RGBA8, tile_size, tile_size, extended ? 32 : 16);
	gl->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	gl->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, upload_format, GL_UNSIGNED_BYTE, data + (y * tile_size * width + x * tile_size) * 4);

			if (extended) {
				gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, upload_format, GL_UNSIGNED_BYTE, data + (y * tile_size * width + (x + 4) * tile_size) * 4);
			}
		}
	}
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	gl->glGenerateTextureMipmap(id);

	gl->glGetTextureSubImage(id, lods - 1, 0, 0, 0, 1, 1, 1, GL_RGBA, GL_FLOAT, 16, &minimap_color);
	minimap_color *= 255.f;

	delete data;
}