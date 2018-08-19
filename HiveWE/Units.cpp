#include "stdafx.h"

bool Units::load(BinaryReader& reader, Terrain& terrain) {
	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::cout << "Invalid war3mapUnits.w3e file: Magic number is not W3do\n";
		return false;
	}
	const uint32_t version = reader.read<uint32_t>();
	if (version != 7 && version != 8) {
		std::cout << "Unknown war3mapUnits.doo version: " << version << " Attempting to load but may crash\nPlease send this map to eejin\n";
	}

	// Subversion
	const int subversion = reader.read<uint32_t>();
	if (subversion != 9 && subversion != 11) {
		std::cout << "Unknown war3mapUnits.doo subversion: " << subversion << " Attempting to load but may crash\nPlease send this map to eejin\n";
	}

	units.resize(reader.read<uint32_t>());
	for (auto&& i : units) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = (reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>() / 128.f;
		
		i.flags = reader.read<uint8_t>();

		i.player = reader.read<uint32_t>();

		i.unknown1 = reader.read<uint8_t>();
		i.unknown2 = reader.read<uint8_t>();

		i.health = reader.read<uint32_t>();
		i.mana = reader.read<uint32_t>();

		if (version >= 8) {
			i.item_table_pointer = reader.read<uint32_t>();
		}

		i.item_sets.resize(reader.read<uint32_t>());
		for (auto&& j : i.item_sets) {
			j.items.resize(reader.read<uint32_t>());
			for (auto&&[id, chance] : j.items) {
				id = reader.read_string(4);
				chance = reader.read<uint32_t>();
			}
		}

		i.gold = reader.read<uint32_t>();
		i.target_acquisition = reader.read<float>();

		i.level = reader.read<uint32_t>();

		if (version >= 8) {
			i.strength = reader.read<uint32_t>();
			i.agility = reader.read<uint32_t>();
			i.intelligence = reader.read<uint32_t>();
		}

		i.items.resize(reader.read<uint32_t>());
		for (auto&& [slot, id] : i.items) {
			slot = reader.read<uint32_t>();
			id = reader.read_string(4);
		}

		i.abilities.resize(reader.read<uint32_t>());
		for (auto&&[id, autocast, level] : i.abilities) {
			id = reader.read_string(4);
			autocast = reader.read<uint32_t>();
			level =  reader.read<uint32_t>();
		}

		i.random_type = reader.read<uint32_t>();
		switch (i.random_type) {
			case 0:
				i.random = reader.read_vector<uint8_t>(4);
				break;
			case 1:
				i.random = reader.read_vector<uint8_t>(8);
				break;
			case 2:
				i.random = reader.read_vector<uint8_t>(reader.read<uint32_t>() * 8);
				break;
		}

		i.custom_color = reader.read<uint32_t>();
		i.waygate = reader.read<uint32_t>();
		i.creation_number = reader.read<uint32_t>();
	}

	units_slk = slk::SLK("Units/UnitData.slk");
	units_slk.merge(slk::SLK("Units/UnitBalance.slk"));
	units_slk.merge(slk::SLK("Units/unitUI.slk"));
	units_slk.merge(slk::SLK("Units/UnitWeapons.slk"));
	units_slk.merge(slk::SLK("Units/UnitAbilities.slk"));

	units_slk.merge(ini::INI("Units/HumanUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/OrcUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/UndeadUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/NightElfUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/NeutralUnitFunc.txt"));

	units_slk.merge(ini::INI("Units/HumanUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/OrcUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/UndeadUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/NightElfUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/NeutralUnitStrings.txt"));

	units_meta_slk = slk::SLK("Units/UnitMetaData.slk");

	items_slk = slk::SLK("Units/ItemData.slk");
	items_slk.merge(ini::INI("Units/ItemFunc.txt"));
	items_slk.merge(ini::INI("Units/ItemStrings.txt"));

	return true;
}

void Units::save() const {
	BinaryWriter writer;

	writer.write_string("W3do");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(write_subversion);

	writer.write<uint32_t>(units.size());
	for (auto&& i : units) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::vec3>(i.position * 128.f + glm::vec3(map.terrain.offset, 0));
		writer.write<float>(i.angle);
		writer.write<glm::vec3>(i.scale * 128.f);

		writer.write<uint8_t>(i.flags);

		writer.write<uint32_t>(i.player);

		writer.write<uint8_t>(i.unknown1);
		writer.write<uint8_t>(i.unknown2);

		writer.write<uint32_t>(i.health);
		writer.write<uint32_t>(i.mana);

		writer.write<uint32_t>(i.item_table_pointer);

		writer.write<uint32_t>(i.item_sets.size());
		for (auto&& j : i.item_sets) {
			writer.write<uint32_t>(j.items.size());
			for (auto&&[id, chance] : j.items) {
				writer.write_string(id);
				writer.write<uint32_t>(chance);
			}
		}

		writer.write<uint32_t>(i.gold);
		writer.write<float>(i.target_acquisition);
		writer.write<uint32_t>(i.level);
		writer.write<uint32_t>(i.strength);
		writer.write<uint32_t>(i.agility);
		writer.write<uint32_t>(i.intelligence);


		writer.write<uint32_t>(i.items.size());
		for (auto&&[slot, id] : i.items) {
			writer.write<uint32_t>(slot);
			writer.write_string(id);
		}

		writer.write<uint32_t>(i.abilities.size());
		for (auto&&[id, autocast, level] : i.abilities) {
			writer.write_string(id);
			writer.write<uint32_t>(autocast);
			writer.write<uint32_t>(level);
		}

		writer.write<uint32_t>(i.random_type);
		writer.write_vector(i.random);

		writer.write<uint32_t>(i.custom_color);
		writer.write<uint32_t>(i.waygate);
		writer.write<uint32_t>(i.creation_number);
	}

	HANDLE handle;
	bool success = SFileCreateFile(hierarchy.map.handle, "war3mapUnits.doo", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	success = SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	if (!success) {
		std::cout << "Writing to file failed: " << GetLastError() << "\n";
	}
	success = SFileFinishFile(handle);
	if (!success) {
		std::cout << "Finishing write failed: " << GetLastError() << "\n";
	}
}

void Units::load_unit_modifications(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown unit modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, units_slk, units_meta_slk, false);
	load_modification_table(reader, units_slk, units_meta_slk, true);
}

void Units::load_item_modifications(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown item modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, items_slk, units_meta_slk, false);
	load_modification_table(reader, items_slk, units_meta_slk, true);
}

void Units::update_area(const QRect& area) {
	for (auto&& i : tree.query(area)) {
		i->position.z = map.terrain.corner_height(i->position.x, i->position.y);
		i->matrix = glm::translate(glm::mat4(1.f), i->position);
		i->matrix = glm::scale(i->matrix, i->scale);
		i->matrix = glm::rotate(i->matrix, i->angle, glm::vec3(0, 0, 1));
	}
}

void Units::create() {
	for (auto&& i : units) {
		i.matrix = glm::translate(i.matrix, i.position);
		i.matrix = glm::scale(i.matrix, i.scale);
		i.matrix = glm::rotate(i.matrix, i.angle, glm::vec3(0, 0, 1));

		tree.insert(&i);

		if (i.id == "sloc") {
			continue;
		} // ToDo handle starting locations
		if (id_to_mesh.find(i.id) != id_to_mesh.end()) {
			continue;
		}

		fs::path mesh_path = units_slk.data("file", i.id);
		if (mesh_path.empty()) {
			mesh_path = items_slk.data("file", i.id);
		}
		mesh_path.replace_extension(".mdx");

		// Mesh doesnt exist at all
		if (!hierarchy.file_exists(mesh_path)) {
			std::cout << "Invalid model file for " << i.id << " With file path: " << mesh_path << "\n";
			id_to_mesh.emplace(i.id, resource_manager.load<StaticMesh>("Objects/Invalidmodel/Invalidmodel.mdx"));
			continue;
		}

		id_to_mesh.emplace(i.id, resource_manager.load<StaticMesh>(mesh_path));
	}
}

void Units::render() {
	for (auto&& i : units) {
		if (i.id == "sloc") {
			continue;
		} // ToDo handle starting locations

		id_to_mesh[i.id]->render_queue(i.matrix);
	}
}