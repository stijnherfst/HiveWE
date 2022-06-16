#include "Sounds.h"

#include <iostream>

//#include "Hierarchy.h"
import Hierarchy;


void Sounds::load() {
	BinaryReader reader = hierarchy.map_file_read("war3map.w3s");

	int version = reader.read<uint32_t>();
	if (version != 1 && version != 2 && version != 3) {
		std::cout << "Unknown war3map.w3s version: " << version << " Attempting to load but may crash\n";
	}

	sounds.resize(reader.read<uint32_t>());
	for (auto& i : sounds) {
		i.name = reader.read_c_string();
		i.file = reader.read_c_string();;
		i.eax_effect = reader.read_c_string();;
		int flags = reader.read<uint32_t>();
		i.looping = flags & 0b00000001;
		i.is_3d = flags & 0b00000010;
		i.stop_out_of_range = flags & 0b00000100;
		i.music = flags & 0b00001000;

		i.fade_in_rate = reader.read<uint32_t>();
		i.fade_out_rate = reader.read<uint32_t>();
		i.volume = reader.read<uint32_t>();
		i.pitch = reader.read<float>();
		i.pitch_variance = reader.read<float>();
		i.priority = reader.read<uint32_t>();
		i.channel = reader.read<uint32_t>();
		i.min_distance = reader.read<float>();
		i.max_distance = reader.read<float>();
		i.distance_cutoff = reader.read<float>();
		i.cone_inside = reader.read<float>();
		i.cone_outside = reader.read<float>();
		i.cone_outside_volume = reader.read<uint32_t>();
		i.cone_orientation_x = reader.read<float>();
		i.cone_orientation_y = reader.read<float>();
		i.cone_orientation_z = reader.read<float>();

		if (version >= 2) {
			// Sound asset data merged with sound data in v2?

			reader.advance_c_string();
			reader.advance_c_string();
			reader.advance_c_string();
			reader.advance(4); // int?
			reader.advance_c_string();
			reader.advance(4); // int?
			if (reader.read<uint32_t>())
				reader.advance_c_string();
			reader.advance_c_string();
			reader.advance_c_string();
			reader.advance_c_string();
			reader.advance_c_string();
			reader.advance_c_string();
			if (version >= 3)
				reader.advance(4); // int?
		}
	}
}

void Sounds::save() const {

}