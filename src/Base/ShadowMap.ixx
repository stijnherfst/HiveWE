module;

#include <glad/glad.h>
#include <vector>
//#include "Globals.h"

export module ShadowMap;

import Hierarchy;
import BinaryReader;

export class ShadowMap {
	size_t width;
	size_t height;

	GLuint texture;
	std::vector<uint8_t> cells;

	bool load(BinaryReader& reader, size_t terrain_width, size_t terrain_height) {
		width = terrain_width * 4;
		height = terrain_height * 4;

		cells = reader.read_vector<uint8_t>(width * height);

		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, 1, GL_R8UI, width, height);
		glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, cells.data());
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		return true;
	}

	void save() const {
		hierarchy.map_file_write("war3map.shd", cells);
	}

	void resize(size_t width, size_t height) {
	
	}
};