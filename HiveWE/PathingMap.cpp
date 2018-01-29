#include "stdafx.h"

bool PathingMap::load(BinaryReader& reader, Terrain& terrain) {
	std::string magic_number = reader.read_string(4);
	if (magic_number != "MP3W") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
		return false;
	}

	int version = reader.read<uint32_t>();
	if (version != 0) {
		std::cout << "Unknown Pathmap version. Attempting to load, but may crash.";
	}

	width = reader.read<uint32_t>();
	height = reader.read<uint32_t>();

	pathing_cells = reader.read_vector<uint8_t>(width * height);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &pathing_texture);
	gl->glTextureStorage2D(pathing_texture, 1, GL_R8UI, width, height);
	gl->glTextureSubImage2D(pathing_texture, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells.data());
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	terrain.pathing_map_texture = pathing_texture;

	return true;
}