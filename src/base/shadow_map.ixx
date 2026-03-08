export module ShadowMap;

import std;
import types;
import Hierarchy;
import BinaryReader;
import <glad/glad.h>;

export class ShadowMap {
	size_t width;
	size_t height;

	GLuint texture;
	std::vector<u8> cells;

  private:
	void update_texture() {
		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureStorage2D(texture, 1, GL_R8UI, width, height);
		glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, cells.data());
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

  public:
	bool load(size_t terrain_width, size_t terrain_height) {
		BinaryReader reader = hierarchy.map_file_read("war3map.shd").value();

		width = terrain_width * 4;
		height = terrain_height * 4;

		// check if the shadow map is correct size
		const size_t expected_size = width * height;
		const size_t file_size = reader.buffer.size();
		if (file_size != expected_size) {
			std::println("Error: Shadow map file size mismatch!");
			cells.resize(expected_size, 0);
		} else {
			cells = reader.read_vector<u8>(expected_size);
		}

		update_texture();

		return true;
	}

	void save() const {
		hierarchy.map_file_write("war3map.shd", cells);
	}

	void resize(size_t new_width, size_t new_height) {
		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);

		glDeleteTextures(1, &texture);
		update_texture();
	}

	void resize(int delta_left, int delta_right, int delta_top, int delta_bottom) {
		size_t new_width = static_cast<size_t>(static_cast<int>(width) + delta_left + delta_right);
		size_t new_height = static_cast<size_t>(static_cast<int>(height) + delta_top + delta_bottom);

		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);

		glDeleteTextures(1, &texture);
		update_texture();
	}
};
