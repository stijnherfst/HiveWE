export module ModificationTables;

import std;
import std.compat;
import BinaryReader;
import BinaryWriter;
import Hierarchy;
import SLK;
import Utilities;
import UnorderedMap;
import <cassert>;

namespace fs = std::filesystem;

constexpr int mod_table_write_version = 3;

void load_modification_table(BinaryReader& reader, const uint32_t version, slk::SLK& slk, const slk::SLK& meta_slk, const bool modification, const bool optional_ints) {
	const uint32_t objects = reader.read<uint32_t>();
	for (size_t i = 0; i < objects; i++) {
		const std::string original_id = reader.read_string(4);
		const std::string modified_id = reader.read_string(4);

		if (version >= 3) {
			const uint32_t set_count = reader.read<uint32_t>();
			if (set_count > 1) {
				std::println("Set count of {} detected", set_count);
			}
			const uint32_t set_flag = reader.read<uint32_t>();
		}
		if (modification && !slk.base_data.contains(modified_id)) {
			slk.copy_row(original_id, modified_id, false);
		}

		const uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			const std::string modification_id = reader.read_string(4);
			const uint32_t type = reader.read<uint32_t>();

			std::string column_header = to_lowercase_copy(meta_slk.data<std::string_view>("field", modification_id));
			if (optional_ints) {
				const uint32_t level_variation = reader.read<uint32_t>();
				const uint32_t data_pointer = reader.read<uint32_t>();
				if (data_pointer != 0) {
					column_header += static_cast<char>('a' + data_pointer - 1);
				}
				if (meta_slk.data<std::string_view>("repeat", modification_id) == "1") {
					column_header += std::to_string(level_variation);
				}
			}

			std::string data;
			switch (type) {
				case 0:
					data = std::to_string(reader.read<int>());
					break;
				case 1:
				case 2:
					data = std::to_string(reader.read<float>());
					break;
				case 3:
					data = reader.read_c_string();
					break;
				default:
					std::println("Unknown data type {} while loading modification table.", type);
			}
			reader.advance(4);

			if (column_header == "") {
				std::println("Unknown mod id: {}", modification_id);
				continue;
			}

			if (modification) {
				slk.set_shadow_data(column_header, modified_id, data);
			} else {
				slk.set_shadow_data(column_header, original_id, data);
			}
		}
	}
}

export void load_modification_file(const std::string_view file_name, slk::SLK& base_data, const slk::SLK& meta_slk, const bool optional_ints) {
	BinaryReader reader = hierarchy.map_file_read(file_name).value();

	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2 && version != 3) {
		std::cout << "Unknown modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, version, base_data, meta_slk, false, optional_ints);
	load_modification_table(reader, version, base_data, meta_slk, true, optional_ints);
}

// The idea of SLKs and mod files is quite bad, but I can deal with them
// The way they are implemented is horrible though
void save_modification_table(BinaryWriter& writer, const slk::SLK& slk, const slk::SLK& meta_slk, const bool custom, const bool optional_ints, const bool skin) {
	BinaryWriter sub_writer;

	size_t count = 0;
	for (const auto& [id, properties] : slk.shadow_data) {
		// If we are writing custom objects then we only want rows with oldid set as the others are base rows
		if (!custom && properties.contains("oldid")) {
			continue;
		} else if (custom && !properties.contains("oldid")) {
			continue;
		}
		count++;

		if (custom) {
			sub_writer.write_string(properties.at("oldid"));
			sub_writer.write_string(id);
		} else {
			sub_writer.write_string(id);
			sub_writer.write<uint32_t>(0);
		}

		// 1.33 fields not yet researched
		sub_writer.write<uint32_t>(1); // sets count
		sub_writer.write<uint32_t>(0); // set flags

		// Split properties, or another way to save war3mapSkin.w3* files correctly?
		// netsafe slk field is probably what vanilla uses (if anyone wants to try this)
		// No changes in skin files and all in main ones is a valid state, but not nice
		if (skin) {
			sub_writer.write<uint32_t>(0); // "no changes"
			continue;
		}
		sub_writer.write<uint32_t>(properties.size() - (properties.contains("oldid") ? 1 : 0));

		for (const auto& [property_id, value] : properties) {
			// The oldis property is a HiveWE specific field of storing the ID a unit/ability is based on
			if (property_id == "oldid") {
				continue;
			}

			int variation = 0;
			int data_pointer = 0;

			size_t nr_position = property_id.find_first_of("0123456789");
			if (nr_position != std::string::npos) {
				variation = std::stoi(property_id.substr(nr_position));
			}

			// If it is a data field then it will contain a data_pointer/column at the end
			if (property_id.starts_with("data")) {
				data_pointer = property_id[4] - 'a' + 1;
			}

			const auto meta_id2 = slk.field_to_meta_id(meta_slk, property_id, id);
			if (!meta_id2) {
				std::println("Meta data key not found for id {} property {}", id, property_id);
				exit(0);
			}
			const std::string meta_data_key = std::string(meta_id2.value());

			sub_writer.write_string(meta_data_key);
			// There's an error in AbilityMetaData.slk where Crs1 instead uses Crs so we need to pad till 4 characters
			for (size_t i = 0; i < 4 - meta_data_key.size(); i++) {
				sub_writer.write<uint8_t>('\0');
			}

			int write_type = -1;
			const std::string_view type = meta_slk.data<std::string_view>("type", meta_data_key);
			if (type == "int" || type == "bool" || type.ends_with("Flags") || type == "attackBits" || type == "channelType" || type == "deathType" || type == "defenseTypeInt" || type == "detectionType" || type == "spellDetail" || type == "teamColor" || type == "techAvail") {
				write_type = 0;
			} else if (type == "real") {
				write_type = 1;
			} else if (type == "unreal") {
				write_type = 2;
			} else { // string
				write_type = 3;
			}

			sub_writer.write<uint32_t>(write_type);

			if (optional_ints) {
				sub_writer.write<uint32_t>(variation);
				sub_writer.write<uint32_t>(data_pointer);
			}

			if (write_type == 0) {
				sub_writer.write<int>(std::stoi(value));
			} else if (write_type == 1 || write_type == 2) {
				sub_writer.write<float>(std::stof(value));
			} else {
				sub_writer.write_c_string(value);
			}

			sub_writer.write<uint32_t>(0);
		}
	}

	writer.write<uint32_t>(count);
	writer.write_vector(sub_writer.buffer);
}

export void save_modification_file(const std::string_view file_name, const slk::SLK& slk, const slk::SLK& meta_slk, const bool optional_ints, const bool skin) {
	BinaryWriter writer;
	writer.write<uint32_t>(mod_table_write_version);

	save_modification_table(writer, slk, meta_slk, false, optional_ints, skin);
	save_modification_table(writer, slk, meta_slk, true, optional_ints, skin);

	hierarchy.map_file_write(file_name, writer.buffer);
}
