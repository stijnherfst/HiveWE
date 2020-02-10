#include "GroundTexture.h"

#include <SOIL2/SOIL2.h>

#include "BLP.h"
#include "Hierarchy.h"

GroundTexture::GroundTexture(const fs::path& path) {
	BinaryReader reader = hierarchy.open_file(path);

	int width;
	int height;
	int channels;
	uint8_t* data;

	if (path.extension() == ".blp" || path.extension() == ".BLP") {
		data = blp::load(reader, width, height, channels);
	} else {
		data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
	}

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
			gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + x * tile_size) * 4);

			if (extended) {
				gl->glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + (x + 4) * tile_size) * 4);
			}
		}
	}
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	gl->glGenerateTextureMipmap(id);

	delete data;
}