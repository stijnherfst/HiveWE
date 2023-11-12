#include "units.h"


#include <filesystem>
#include <iostream>
#include <print>

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "globals.h"
#include <map_global.h>

import BinaryWriter;
import Hierarchy;

void Unit::update() {
	float model_scale = 0.f;
	float move_height = 0.f;
	if (items_slk.row_headers.contains(id)) {
		model_scale = items_slk.data<float>("scale", id) / 128.f;
		color.r = items_slk.data<float>("colorr", id) / 255.f;
		color.g = items_slk.data<float>("colorg", id) / 255.f;
		color.b = items_slk.data<float>("colorb", id) / 255.f;
	} else {
		model_scale = units_slk.data<float>("modelscale", id) / 128.f;
		move_height = units_slk.data<float>("moveheight", id) / 128.f;
		color.r = units_slk.data<float>("red", id) / 255.f;
		color.g = units_slk.data<float>("green", id) / 255.f;
		color.b = units_slk.data<float>("blue", id) / 255.f;
	}

	const glm::vec3 final_position = position + glm::vec3(0.f, 0.f, move_height);
	const glm::vec3 final_scale = glm::vec3(model_scale);

	skeleton.update_location(final_position, angle, final_scale);
}

void Units::load() {
	BinaryReader reader = hierarchy.map_file_read("war3mapUnits.doo");

	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::cout << "Invalid war3mapUnits.w3e file: Magic number is not W3do\n";
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
		i.position = (reader.read<glm::vec3>() - glm::vec3(map->terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>() / 128.f;

		if (map->info.game_version_major * 100 + map->info.game_version_minor >= 132) {
			i.skin_id = reader.read_string(4);
		} else {
			i.skin_id = i.id;
		}

		i.flags = reader.read<uint8_t>();

		i.player = reader.read<uint32_t>();
		if (i.player > 11 && map->info.editor_version < 6060) {
			i.player += 12;
		}

		i.unknown1 = reader.read<uint8_t>();
		i.unknown2 = reader.read<uint8_t>();

		i.health = reader.read<uint32_t>();
		i.mana = reader.read<uint32_t>();

		if (subversion >= 11) {
			i.item_table_pointer = reader.read<uint32_t>();
		}

		i.item_sets.resize(reader.read<uint32_t>());
		for (auto&& j : i.item_sets) {
			j.items.resize(reader.read<uint32_t>());
			for (auto&&[chance, id] : j.items) {
				id = reader.read_string(4);
				chance = reader.read<uint32_t>();
			}
		}

		i.gold = reader.read<uint32_t>();
		i.target_acquisition = reader.read<float>();

		i.level = reader.read<uint32_t>();

		if (subversion >= 11) {
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
		if (units_slk.row_headers.contains(i.id) || i.id == "sloc" || i.id == "uDNR" || i.id == "bDNR") {
			units.push_back(i);
		} else {
			items.push_back(i);
		}

		Unit::auto_increment = std::max(Unit::auto_increment, i.creation_number);
	}
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

			writer.write_string(i.skin_id);

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
				for (auto&&[chance, id] : j.items) {
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

void Units::update_area(const QRect& area) {
	for (auto&& i : query_area(area)) {
		i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y, true);
		i->update();
	}
}

void Units::create() {
	for (auto& i : units) {
		// ToDo handle starting location
		if (i.id == "sloc") {
			continue;
		}

		i.mesh = get_mesh(i.id);
		i.skeleton = SkeletalModelInstance(i.mesh->model);
		i.update();
	}	

	for (auto& i : items) {
		i.mesh = get_mesh(i.id);
		i.skeleton = SkeletalModelInstance(i.mesh->model);
		i.update();
	}
}

// Will assign a unique creation number
Unit& Units::add_unit(std::string id, glm::vec3 position) {
	// ToDo change this once SkeletalModelInstance doesn't use pointers anymore
	units.push_back(Unit());
	Unit& unit = units.back();
	unit.id = id;
	unit.skin_id = id;
	unit.mesh = get_mesh(id);
	unit.position = position;
	unit.scale = glm::vec3(1.f);
	unit.angle = 0.f;
	unit.random = { 1, 0, 0, 0 };
	unit.creation_number = ++Unit::auto_increment;
	unit.skeleton = SkeletalModelInstance(unit.mesh->model);
	unit.update();

	return units.back();
}

// Assumes you will set a unique creation number yourself
Unit& Units::add_unit(Unit unit) {
	units.push_back(unit);
	return units.back();
}

void Units::remove_unit(Unit* unit) {
	auto iterator = units.begin() + std::distance(units.data(), unit);
	units.erase(iterator);
}

std::vector<Unit*> Units::query_area(const QRectF& area) {
	std::vector<Unit*> result;

	for (auto& i : units) {
		if (area.contains(i.position.x, i.position.y) && i.id != "sloc") {
			result.push_back(&i);
		}
	}
	return result;
}

void Units::remove_units(const std::unordered_set<Unit*>& list) {
	std::erase_if(units, [&](Unit& unit) {
		return list.contains(&unit);
	});
}

void Units::process_unit_field_change(const std::string& id, const std::string& field) {
	if (field == "file") {
		id_to_mesh.erase(id);
		for (auto& i : units) {
			if (i.id == id) {
				i.mesh = get_mesh(id);
				i.skeleton = SkeletalModelInstance(i.mesh->model);
				i.update();
			}
		}
	}
	if (field == "modelscale" || field == "moveheight" || field == "red" || field == "green" || field == "blue") {
		for (auto& i : units) {
			if (i.id == id) {
				i.update();
			}
		}
	}

	if (field == "colorr" || field == "colorg" || field == "colorb" || field == "scale") {
		for (auto& i : items) {
			if (i.id == id) {
				i.update();
			}
		}
	}
}

void Units::process_item_field_change(const std::string& id, const std::string& field) {
	if (field == "colorr" || field == "colorg" || field == "colorb" || field == "scale") {
		for (auto& i : items) {
			if (i.id == id) {
				i.update();
			}
		}
	}
}

std::shared_ptr<SkinnedMesh> Units::get_mesh(const std::string& id) {
	if (id_to_mesh.find(id) != id_to_mesh.end()) {
		return id_to_mesh[id];
	}

	fs::path mesh_path = units_slk.data("file", id);
	if (mesh_path.empty()) {
		mesh_path = items_slk.data("file", id);
	}
	mesh_path.replace_extension(".mdx");

	mesh_path = fs::path(string_replaced(mesh_path.string(), "\\", "/"));

	// Mesh doesnt exist at all
	if (!hierarchy.file_exists(mesh_path)) {
		std::cout << "Invalid model file for " << id << " With file path: " << mesh_path << "\n";
		id_to_mesh.emplace(id, resource_manager.load<SkinnedMesh>("Objects/Invalidmodel/Invalidmodel.mdx", "", std::nullopt));
		return id_to_mesh[id];
	}

	id_to_mesh.emplace(id, resource_manager.load<SkinnedMesh>(mesh_path, "", std::nullopt));

	return id_to_mesh[id];
}

void UnitAddAction::undo() {
	map->units.units.resize(map->units.units.size() - units.size());
}

void UnitAddAction::redo() {
	map->units.units.insert(map->units.units.end(), units.begin(), units.end());
}

void UnitDeleteAction::undo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->units.units.insert(map->units.units.end(), units.begin(), units.end());
}

void UnitDeleteAction::redo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->units.units.resize(map->units.units.size() - units.size());
}

void UnitStateAction::undo() {
	for (auto& i : old_units) {
		for (auto& j : map->units.units) {
			if (i.creation_number == j.creation_number) {
				j = i;
			}
		}
	}
}

void UnitStateAction::redo() {
	for (auto& i : new_units) {
		for (auto& j : map->units.units) {
			if (i.creation_number == j.creation_number) {
				j = i;
			}
		}
	}
}
