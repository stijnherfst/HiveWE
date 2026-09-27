export module ShadowMap;

import std;
import types;
import Hierarchy;
import BinaryReader;

export class ShadowMap {
	size_t width;
	size_t height;

	std::vector<u8> cells;

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

		return true;
	}

	void save() const {
		hierarchy.map_file_write("war3map.shd", cells);
	}

	void resize(const size_t new_width, const size_t new_height) {
		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);
	}

	void resize(const int delta_left, const int delta_right, const int delta_top, const int delta_bottom) {
		const size_t new_width = static_cast<size_t>(static_cast<int>(width) + delta_left + delta_right);
		const size_t new_height = static_cast<size_t>(static_cast<int>(height) + delta_top + delta_bottom);

		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);
	}
};
