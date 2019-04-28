#include "stdafx.h"

bool ShadowMap::load(BinaryReader& reader) {
	width = map->terrain.width * 4;
	height = map->terrain.height * 4;

	cells = reader.read_vector<uint8_t>(width * height);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	gl->glTextureStorage2D(texture, 1, GL_R8UI, width, height);
	gl->glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, cells.data());
	gl->glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;
}

void ShadowMap::save() const {
	BinaryWriter writer;
	writer.write_vector<uint8_t>(cells);

	hierarchy.map_file_write("war3map.shd", writer.buffer);
}