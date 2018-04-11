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
	reader.read<uint32_t>();

	const int unit_count = reader.read<uint32_t>();

	units.resize(unit_count);
	for (int i = 0; i < unit_count; i++) {
		units[i].id = reader.read_string(4);
		units[i].variation = reader.read<uint32_t>();
		units[i].position = reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0);
		units[i].angle = reader.read<float>();
		units[i].scale = reader.read<glm::vec3>();
		
		// flags
		reader.read<uint8_t>();

		units[i].player = reader.read<uint32_t>();

		// Unknwon
		reader.read<uint8_t>();
		reader.read<uint8_t>();

		units[i].health = reader.read<uint32_t>();
		units[i].mana = reader.read<uint32_t>();

		// map item table pointer
		if (version >= 8) {
			reader.read<uint32_t>();
		}

		const int item_sets = reader.read<uint32_t>();

		for (int j = 0; j < item_sets; j++) {
			const int dropable_items = reader.read<uint32_t>();
			for (int k = 0; k < dropable_items; k++) {
				// Item id
				reader.read_string(4);
				// Drop chance
				reader.read<uint32_t>();
			}
		}

		// Gold
		reader.read<uint32_t>();

		// Target acquisition
		reader.read<float>();

		// Hero lvl
		reader.read<uint32_t>();


		if (version >= 8) {
			// Strength
			reader.read<uint32_t>();

			// Agility
			reader.read<uint32_t>();

			// Intelligence
			reader.read<uint32_t>();
		}

		const int items = reader.read<uint32_t>();
		for (int j = 0; j < items; j++) {
			// Inventory slot
			reader.read<uint32_t>();
			// Item ID
			reader.read_string(4);
		}

		const int modified_abilities = reader.read<uint32_t>();
		for (int j = 0; j < modified_abilities; j++) {
			// Ability ID
			reader.read_string(4);
			// Autocast active
			reader.read<uint32_t>();
			// Ability level
			reader.read<uint32_t>();
		}

		// Is random unit/item
		const int random = reader.read<uint32_t>();
		switch (random) {
			case 0:
				reader.position += 4;
				break;
			case 1:
				reader.position += 8;
				break;
			case 2:
				reader.position += reader.read<uint32_t>() * 8;
				break;
			default: 
				std::cout << "Unknown random " << random << " while loading units\n";
				break;
		}

		int custom_color = reader.read<uint32_t>();
		int waygate = reader.read<uint32_t>();
		int creation_number = reader.read<uint32_t>();
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

void Units::create() {
	for (auto&& i : units) {
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
		glm::mat4 model = glm::translate(glm::mat4(1.0f), i.position / 128.f);
		model = glm::scale(model, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f) * i.scale);
		model = glm::rotate(model, i.angle, glm::vec3(0, 0, 1));

		id_to_mesh[i.id]->render_queue(camera.projection_view * model);
	}
}