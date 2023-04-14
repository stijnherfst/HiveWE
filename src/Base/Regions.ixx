module;

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <iostream>

export module Regions;

import BinaryReader;
import Hierarchy;

struct Region {
	float left;
	float right;
	float top;
	float bottom;
	std::string name;
	int creation_number;
	std::string weather_id;
	std::string ambient_id;
	glm::vec3 color;
};

export class Regions {
	static constexpr int write_version = 5;

  public:
	std::vector<Region> regions;

	bool load() {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3r");

		const int version = reader.read<uint32_t>();
		if (version != 5) {
			std::cout << "Unknown Regions file version. Attempting to load, but may crash.";
		}

		regions.resize(reader.read<uint32_t>());
		for (auto& i : regions) {
			i.left = reader.read<float>();
			i.bottom = reader.read<float>();
			i.right = reader.read<float>();
			i.top = reader.read<float>();
			i.name = reader.read_c_string();
			i.creation_number = reader.read<int>();
			i.weather_id = reader.read_string(4);
			i.ambient_id = reader.read_c_string();
			i.color = reader.read<glm::u8vec3>();
			reader.advance(1); // padding?
		}

		return true;
	}

	void save() const {
		//	BinaryWriter writer;
		// writer.write_string("MP3W");
		// writer.write<uint32_t>(write_version);
		// writer.write<uint32_t>(width);
		// writer.write<uint32_t>(height);
		// writer.write_vector<uint8_t>(pathing_cells_static);

		//	hierarchy.map_file_write("war3map.wpr", writer.buffer);
	}
};