#include "stdafx.h"

void Unit::update() {
	matrix = glm::translate(glm::mat4(1.f), position);
	matrix = glm::scale(matrix, scale / 128.f);
	matrix = glm::rotate(matrix, angle, glm::vec3(0, 0, 1));
}

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

	const int unit_count = reader.read<uint32_t>();
	for (int k = 0; k < unit_count; k++) {
		Unit i;
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = (reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();

		// Scale isn't actually used
		//i.scale = reader.read<glm::vec3>() / 128.f;
		reader.advance(12);
		i.scale = glm::vec3(1.f);
		
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

		// Either a unit or an item
		if (units_slk.row_header_exists(i.id) || i.id == "sloc" || i.id == "uDNR" || i.id == "bDNR") {
			units.push_back(i);
		} else {
			items.push_back(i);
		}
	}

	return true;
}

void Units::save() const {
	BinaryWriter writer;

	writer.write_string("W3do");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(write_subversion);

	writer.write<uint32_t>(units.size() + items.size());

	auto write_units = [&](const std::vector<Unit>& to_write) {
		for (auto&& i : to_write) {
			writer.write_string(i.id);
			writer.write<uint32_t>(i.variation);
			writer.write<glm::vec3>(i.position * 128.f + glm::vec3(map->terrain.offset, 0));
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
	};

	write_units(units);
	write_units(items);

	hierarchy.map_file_write("war3mapUnits.doo", writer.buffer);
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
		i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y);
		i->update();
	}
}

void Units::create() {
	for (auto&& i : units) {
		// ToDo handle starting location
		if (i.id == "sloc") {
			continue;
		}
		// ToDo handle random units
		if (i.id != "uDNR" && i.id != "bDNR") {
			i.scale = glm::vec3(std::stof(units_slk.data("modelScale", i.id)));
		}

		i.update();

		tree.insert(&i);
		i.mesh = get_mesh(i.id);
	}	
	for (auto&& i : items) {
		i.scale = glm::vec3(std::stof(items_slk.data("scale", i.id)));

		i.update();

		tree.insert(&i);
		i.mesh = get_mesh(i.id);
	}
}

void Units::render() const {
	for (auto&& i : units) {
		if (i.id == "sloc") {
			continue;
		} // ToDo handle starting locations

		i.mesh->render_queue(i.matrix);
	}
	for (auto&& i : items) {
		i.mesh->render_queue(i.matrix);
	}
}

std::shared_ptr<StaticMesh> Units::get_mesh(const std::string& id) {
	if (id_to_mesh.contains(id)) {
		return id_to_mesh[id];
	}

	fs::path mesh_path = units_slk.data("file", id);
	if (mesh_path.empty()) {
		mesh_path = items_slk.data("file", id);
	}
	mesh_path.replace_extension(".mdx");

	// Mesh doesnt exist at all
	if (!hierarchy.file_exists(mesh_path)) {
		std::cout << "Invalid model file for " << id << " With file path: " << mesh_path << "\n";
		id_to_mesh.emplace(id, resource_manager.load<StaticMesh>("Objects/Invalidmodel/Invalidmodel.mdx"));
		return id_to_mesh[id];
	}

	id_to_mesh.emplace(id, resource_manager.load<StaticMesh>(mesh_path));

	return id_to_mesh[id];
}