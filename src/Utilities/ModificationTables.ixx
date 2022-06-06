module;

#include <SLK.h>

export module ModificationTables;

import<vector>;
import<string>;
import<filesystem>;
import<fstream>;
import<iostream>;

import BinaryReader;
import BinaryWriter;


namespace fs = std::filesystem;
//
//export void load_modification_table(BinaryReader& reader, SLK& slk, const SLK& meta_slk, const bool modification, bool optional_ints) {
//	const uint32_t objects = reader.read<uint32_t>();
//	for (size_t i = 0; i < objects; i++) {
//		const std::string original_id = reader.read_string(4);
//		const std::string modified_id = reader.read_string(4);
//
//		if (modification) {
//			slk.copy_row(original_id, modified_id, false);
//		}
//
//		const uint32_t modifications = reader.read<uint32_t>();
//
//		for (size_t j = 0; j < modifications; j++) {
//			const std::string modification_id = reader.read_string(4);
//			const uint32_t type = reader.read<uint32_t>();
//
//			std::string column_header = to_lowercase_copy(meta_slk.data("field", modification_id));
//			if (optional_ints) {
//				uint32_t level_variation = reader.read<uint32_t>();
//				uint32_t data_pointer = reader.read<uint32_t>();
//				if (data_pointer != 0) {
//					column_header += char('a' + data_pointer - 1);
//				}
//				if (level_variation != 0) {
//					column_header += std::to_string(level_variation);
//				}
//
//				// Can remove after checking whether this holds for many maps
//				if (data_pointer != 0 && level_variation == 0) {
//					assert(!(data_pointer != 0 && level_variation == 0));
//				}
//			}
//
//			std::string data;
//			switch (type) {
//				case 0:
//					data = std::to_string(reader.read<int>());
//					break;
//				case 1:
//				case 2:
//					data = std::to_string(reader.read<float>());
//					break;
//				case 3:
//					data = reader.read_c_string();
//					break;
//				default:
//					std::cout << "Unknown data type " << type << " while loading modification table.";
//			}
//			reader.advance(4);
//
//			if (column_header == "") {
//				std::cout << "Unknown mod id: " << modification_id << "\n";
//				continue;
//			}
//
//			if (modification) {
//				slk.set_shadow_data(column_header, modified_id, data);
//			} else {
//				slk.set_shadow_data(column_header, original_id, data);
//			}
//		}
//	}
//}
//
//export void load_modification_file(const fs::path& file_name, SLK& base_data, const SLK& meta_slk, bool optional_ints) {
//	BinaryReader reader = hierarchy.map_file_read(file_name);
//
//	const int version = reader.read<uint32_t>();
//	if (version != 1 && version != 2) {
//		std::cout << "Unknown modification table version of " << version << " detected. Attempting to load, but may crash.\n";
//	}
//
//	load_modification_table(reader, base_data, meta_slk, false, optional_ints);
//	load_modification_table(reader, base_data, meta_slk, true, optional_ints);
//}
//
//// The idea of SLKs and mod files is quite bad, but I can deal with them
//// The way they are implemented is horrible though
//export void save_modification_table(BinaryWriter& writer, const SLK& slk, const SLK& meta_slk, bool custom, bool optional_ints) {
//	// Create an temporary index to speed up field lookups
//	absl::flat_hash_map<std::string, std::string> meta_index;
//	for (const auto& [key, dontcare2] : meta_slk.row_headers) {
//		std::string field = to_lowercase_copy(meta_slk.data("field", key));
//		meta_index[field] = key;
//	}
//
//	BinaryWriter sub_writer;
//
//	size_t count = 0;
//	for (const auto& [id, properties] : slk.shadow_data) {
//		// If we are writing custom objects then we only want rows with oldid set as the others are base rows
//		if (!custom && properties.contains("oldid")) {
//			continue;
//		} else if (custom && !properties.contains("oldid")) {
//			continue;
//		}
//		count++;
//
//		if (custom) {
//			sub_writer.write_string(properties.at("oldid"));
//			sub_writer.write_string(id);
//		} else {
//			sub_writer.write_string(id);
//			sub_writer.write<uint32_t>(0);
//		}
//
//		sub_writer.write<uint32_t>(properties.size() - (properties.contains("oldid") ? 1 : 0));
//
//		const std::string base_id = custom ? properties.at("oldid") : id;
//		std::string meta_id = custom ? properties.at("oldid") : id;
//		if (slk.column_headers.contains("code")) {
//			meta_id = slk.data("code", meta_id);
//		}
//
//		for (const auto& [property_id, value] : properties) {
//			if (property_id == "oldid") {
//				continue;
//			}
//
//			// Find the metadata ID for this field name since modification files are stupid
//			std::string meta_data_key;
//
//			int variation = 0;
//			int data_pointer = 0;
//			if (meta_index.contains(property_id)) {
//				meta_data_key = meta_index.at(property_id);
//			} else {
//				// First strip off the variation/level
//				size_t nr_position = property_id.find_first_of("0123456789");
//				std::string without_numbers = property_id.substr(0, nr_position);
//
//				if (nr_position != std::string::npos) {
//					variation = std::stoi(property_id.substr(nr_position));
//				}
//
//				if (meta_index.contains(without_numbers)) {
//					meta_data_key = meta_index.at(without_numbers);
//				} else {
//					// If it is a data field then it will contain a data_pointer/column at the end
//					if (without_numbers.starts_with("data")) {
//						data_pointer = without_numbers[4] - 'a' + 1;
//						without_numbers = "data";
//					}
//
//					if (without_numbers == "data" || without_numbers == "unitid" || without_numbers == "cast") {
//						// Unfortunately mapping a data field to a key is not easy so we have to iterate over the entire meta_slk
//						for (const auto& [key, dontcare2] : meta_slk.row_headers) {
//							if (meta_slk.data<int>("data", key) != data_pointer) {
//								continue;
//							}
//
//							if (to_lowercase_copy(meta_slk.data("field", key)) != without_numbers) {
//								continue;
//							}
//
//							std::string use_specific = meta_slk.data("usespecific", key);
//							std::string not_specific = meta_slk.data("notspecific", key);
//
//							// If we are in the exclude list
//							if (not_specific.find(meta_id) != std::string::npos) {
//								continue;
//							}
//
//							// If the include list is not empty and we are not inside
//							if (!use_specific.empty() && use_specific.find(meta_id) == std::string::npos && use_specific.find(base_id) == std::string::npos) {
//								continue;
//							}
//
//							meta_data_key = key;
//							break;
//						}
//					}
//				}
//
//				if (meta_data_key.empty()) {
//					puts("Empty meta data key");
//					exit(0);
//				}
//			}
//
//			if (meta_data_key.empty()) {
//				puts("Empty meta data key");
//				exit(0);
//			}
//
//			sub_writer.write_string(meta_data_key);
//
//			int write_type = -1;
//			const std::string type = meta_slk.data("type", meta_data_key);
//			if (type == "int" || type == "bool" || type.ends_with("Flags") || type == "attackBits" || type == "channelType" || type == "deathType" || type == "defenseTypeInt" || type == "detectionType" || type == "spellDetail" || type == "teamColor" || type == "techAvail") {
//
//				write_type = 0;
//			} else if (type == "real") {
//				write_type = 1;
//			} else if (type == "unreal") {
//				write_type = 2;
//			} else { // string
//				write_type = 3;
//			}
//
//			sub_writer.write<uint32_t>(write_type);
//
//			if (optional_ints) {
//				sub_writer.write<uint32_t>(variation);
//				sub_writer.write<uint32_t>(data_pointer);
//			}
//
//			if (write_type == 0) {
//				sub_writer.write<int>(std::stoi(value));
//			} else if (write_type == 1 || write_type == 2) {
//				sub_writer.write<float>(std::stof(value));
//			} else {
//				sub_writer.write_c_string(value);
//			}
//
//			sub_writer.write<uint32_t>(0);
//		}
//	}
//
//	writer.write<uint32_t>(count);
//	writer.write_vector(sub_writer.buffer);
//}
//
//export void save_modification_file(const fs::path& file_name, const SLK& slk, const SLK& meta_slk, bool optional_ints) {
//	BinaryWriter writer;
//	writer.write<uint32_t>(mod_table_write_version);
//
//	save_modification_table(writer, slk, meta_slk, false, optional_ints);
//	save_modification_table(writer, slk, meta_slk, true, optional_ints);
//
//	hierarchy.map_file_write(file_name, writer.buffer);
//}