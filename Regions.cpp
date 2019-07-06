#include "stdafx.h"

bool Regions::load(BinaryReader& reader) {
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

void Regions::save() const {
	BinaryWriter writer;
	//writer.write_string("MP3W");
	//writer.write<uint32_t>(write_version);
	//writer.write<uint32_t>(width);
	//writer.write<uint32_t>(height);
	//writer.write_vector<uint8_t>(pathing_cells_static);

	hierarchy.map_file_write("war3map.wpr", writer.buffer);
}