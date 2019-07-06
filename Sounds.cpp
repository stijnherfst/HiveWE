#include "stdafx.h"

void Sounds::load(BinaryReader& reader) {
	int version = reader.read<uint32_t>();
	if (version != 1) {
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
		i.unknown = reader.read<float>();
		i.unknown2 = reader.read<uint32_t>();
		i.channel = reader.read<uint32_t>();
		i.min_distance = reader.read<float>();
		i.max_distance = reader.read<float>();
		i.distance_cutoff = reader.read<float>();
		i.unknown3 = reader.read<float>();
		i.unknown4 = reader.read<float>();
		i.unknown5 = reader.read<uint32_t>();
		i.unknown6 = reader.read<float>();
		i.unknown7 = reader.read<float>();
		i.unknown8 = reader.read<float>();
	}
}

void Sounds::save() const {

}