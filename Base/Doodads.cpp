#include "Doodads.h"

#include <iostream>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "HiveWE.h"
#include "BinaryWriter.h"
#include "Hierarchy.h"

void Doodad::update() {
	glm::vec3 base_scale = glm::vec3(1.f);
	matrix = glm::translate(glm::mat4(1.f), position);
	matrix = glm::scale(matrix, (base_scale - 1.f + scale) / 128.f);
	matrix = glm::rotate(matrix, angle, glm::vec3(0, 0, 1));

	std::string max_roll;
	if (doodads_slk.row_headers.contains(id)) {
		color.r = doodads_slk.data<float>("vertr" + std::to_string(variation + 1), id) / 255.f;
		color.g = doodads_slk.data<float>("vertg" + std::to_string(variation + 1), id) / 255.f;
		color.b = doodads_slk.data<float>("vertb" + std::to_string(variation + 1), id) / 255.f;
		max_roll = doodads_slk.data("maxroll", id);
	} else {
		color.r = destructibles_slk.data<float>("colorr", id) / 255.f;
		color.g = destructibles_slk.data<float>("colorg", id) / 255.f;
		color.b = destructibles_slk.data<float>("colorb", id) / 255.f;
		max_roll = destructibles_slk.data("maxroll", id);
	}

	if (!max_roll.empty()) {
		matrix = glm::rotate(matrix, -std::stof(max_roll), glm::vec3(1, 0, 0));
	}
}

float Doodad::acceptable_angle(std::string_view id, std::shared_ptr<PathingTexture> pathing, float current_angle, float target_angle) {
	bool fixed_rotation = false;
	if (doodads_slk.row_headers.contains(id)) {
		fixed_rotation = doodads_slk.data<int>("fixedrot", id) > 0;
	} else {
		fixed_rotation = destructibles_slk.data<int>("fixedrot", id) > 0;
	}

	if (fixed_rotation) {
		return current_angle;
	}

	if (pathing) {
		if (pathing->width == pathing->height && pathing->homogeneous) {
			return target_angle;
		} else {
			return (static_cast<int>((target_angle + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4) * glm::pi<float>() * 0.5f;
		}
	} else {
		return target_angle;
	}
}


bool Doodads::load() {
	BinaryReader reader = hierarchy.map_file_read("war3map.doo");

	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::cout << "Invalid war3map.doo file: Magic number is not W3do\n";
		return false;
	}
	const uint32_t version = reader.read<uint32_t>();
	if (version != 7 && version != 8) {
		std::cout << "Unknown war3map.doo version: " << version << " Attempting to load but may crash\nPlease send this map to eejin\n";
	}

	// Subversion
	const uint32_t subversion = reader.read<uint32_t>();
	// ToDO check subversion

	Doodad::auto_increment = 0;
	doodads.resize(reader.read<uint32_t>());
	for (auto&& i : doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = (reader.read<glm::vec3>() - glm::vec3(map->terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>();

		if (map->info.game_version_major * 100 + map->info.game_version_minor >= 132) {
			i.skin_id = reader.read_string(4);
		} else {
			i.skin_id = i.id;
		}

		i.state = static_cast<Doodad::State>(reader.read<uint8_t>());
		i.life = reader.read<uint8_t>();

		if (version >= 8) {
			i.item_table_pointer = reader.read<int32_t>();
			i.item_sets.resize(reader.read<uint32_t>());
			for (auto&& j : i.item_sets) {
				j.items.resize(reader.read<uint32_t>());
				for (auto& [id, chance] : j.items) {
					id = reader.read_string(4);
					chance = reader.read<uint32_t>();
				}
			}
		}

		i.creation_number = reader.read<uint32_t>();
		Doodad::auto_increment = std::max(Doodad::auto_increment, i.creation_number);
	}

	// Terrain Doodads
	const int special_format_version = reader.read<uint32_t>();

	special_doodads.resize(reader.read<uint32_t>());
	for (auto&& i : special_doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = glm::ivec3(reader.read<glm::ivec2>(), 0);
	}

	return true;
}

void Doodads::save() const {
	BinaryWriter writer;
	writer.write_string("W3do");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(write_subversion);

	writer.write<uint32_t>(doodads.size());
	for (auto&& i : doodads) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::vec3>(i.position * 128.f + glm::vec3(map->terrain.offset, 0));
		writer.write<float>(i.angle);
		writer.write<glm::vec3>(i.scale);

		writer.write_string(i.skin_id);

		writer.write<uint8_t>(static_cast<int>(i.state));
		writer.write<uint8_t>(i.life);

		writer.write<int32_t>(i.item_table_pointer);
		writer.write<uint32_t>(i.item_sets.size());
		for (auto&& j : i.item_sets) {
			writer.write<uint32_t>(j.items.size());
			for (const auto& [id, chance] : j.items) {
				writer.write_string(id);
				writer.write<uint32_t>(chance);
			}
		}

		writer.write<uint32_t>(i.creation_number);
	}

	writer.write<uint32_t>(write_special_version);

	writer.write<uint32_t>(special_doodads.size());
	for (auto&& i : special_doodads) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::ivec2>(glm::ivec2(i.position.x, i.position.y));
	}

	hierarchy.map_file_write("war3map.doo", writer.buffer);
}

void Doodads::create() {
	for (auto&& i : doodads) {
		i.update();
		i.mesh = get_mesh(i.id, i.variation);

		// Get pathing map
		const bool is_doodad = doodads_slk.row_headers.contains(i.id);
		const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

		const std::string pathing_texture_path = slk.data("pathtex", i.id);
		if (hierarchy.file_exists(pathing_texture_path)) {
			i.pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
		}
	}

	for (auto&& i : special_doodads) {
		float rotation = doodads_slk.data<int>("fixedrot", i.id) / 360.f * 2.f * glm::pi<float>();
		i.matrix = glm::translate(i.matrix, i.position);
		i.matrix = glm::scale(i.matrix, { 1.f / 128.f, 1.f / 128.f, 1.f / 128.f });
		i.matrix = glm::rotate(i.matrix, rotation, glm::vec3(0, 0, 1));

		i.mesh = get_mesh(i.id, i.variation);
		const std::string pathing_texture_path = doodads_slk.data("pathtex", i.id);
		if (hierarchy.file_exists(pathing_texture_path)) {
			i.pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
		}
	}
}

void Doodads::render() {
	for (auto&& i : doodads) {
		i.mesh->render_queue(i.matrix, i.color);
	}
	for (auto&& i : special_doodads) {
		i.mesh->render_queue(i.matrix, glm::vec3(1.f));
	}
}

// Will assign a creation number
Doodad& Doodads::add_doodad(std::string id, int variation, glm::vec3 position) {
	Doodad doodad;
	doodad.id = id;
	doodad.skin_id = id;
	doodad.variation = variation;
	doodad.mesh = get_mesh(id, variation);
	doodad.position = position;
	doodad.scale = { 1, 1, 1 };
	doodad.angle = 0;
	doodad.creation_number = ++Doodad::auto_increment;

	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
	std::string pathing_texture_path = slk.data("pathtex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		doodad.pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
	}

	doodad.update();

	doodads.push_back(doodad);
	return doodads.back();
}

// You will have to manually set a creation number and valid skin ID
Doodad& Doodads::add_doodad(Doodad doodad) {
	doodads.push_back(doodad);
	return doodads.back();
}

void Doodads::remove_doodad(Doodad* doodad) {
	auto iterator = doodads.begin() + std::distance(doodads.data(), doodad);
	doodads.erase(iterator);
}

std::vector<Doodad*> Doodads::query_area(const QRectF& area) {
	std::vector<Doodad*> result;

	for (auto&& i : doodads) {
		if (area.contains(i.position.x, i.position.y)) {
			result.push_back(&i);
		}
	}
	return result;
}

void Doodads::remove_doodads(const std::vector<Doodad*>& list) {
	doodads.erase(std::remove_if(doodads.begin(), doodads.end(), [&](Doodad& doodad) {
					  return std::find(list.begin(), list.end(), &doodad) != list.end();
				  }),
				  doodads.end());
}

void Doodads::update_doodad_pathing(const std::vector<Doodad>& target_doodads) {
	QRectF update_pathing_area;
	for (const auto& i : target_doodads) {
		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	update_doodad_pathing(update_pathing_area);
}

void Doodads::update_doodad_pathing(const std::vector<Doodad*>& target_doodads) {
	QRectF update_pathing_area;
	for (const auto& i : target_doodads) {
		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i->position.x, i->position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i->position.x, i->position.y, 1.f, 1.f };
	}

	update_doodad_pathing(update_pathing_area);
}

void Doodads::update_doodad_pathing(const QRectF& area) {
	QRectF new_area = area.adjusted(-6, -6, 6, 6);
	map->pathing_map.dynamic_clear_area(new_area.toRect());

	new_area.adjust(-6, -6, 6, 6);

	const auto doodads_to_blit = map->doodads.query_area(new_area);
	for (const auto& i : doodads_to_blit) {
		if (!i->pathing) {
			continue;
		}
		map->pathing_map.blit_pathing_texture(i->position, glm::degrees(i->angle) + 90, i->pathing);
	}
	map->pathing_map.upload_dynamic_pathing();
}

void Doodads::process_doodad_field_change(const std::string& id, const std::string& field) {
	if (field == "file" || field == "numvar") {
		// id_to_mesh requires a variation too so we will just have to check a bunch of them
		for (int i = 0; i < 20; i++) {
			if (id_to_mesh.contains(id + std::to_string(i))) {
				id_to_mesh.erase(id_to_mesh.find(id));
			}
		}
		for (auto& i : doodads) {
			if (i.id == id) {
				i.mesh = get_mesh(id, i.variation);
				i.update();
			}
		}
	}

	if (field == "maxroll" || field == "vertr" || field == "vertg" || field == "vertb") {
		for (auto& i : doodads) {
			if (i.id == id) {
				i.update();
			}
		}
	}
}

void Doodads::process_destructible_field_change(const std::string& id, const std::string& field) {
	if (field == "file" || field == "numvar") {
		// id_to_mesh requires a variation too so we will just have to check a bunch of them
		for (int i = 0; i < 20; i++) {
			if (id_to_mesh.contains(id + std::to_string(i))) {
				id_to_mesh.erase(id_to_mesh.find(id + std::to_string(i)));
			}
		}
		for (auto& i : doodads) {
			if (i.id == id) {
				i.mesh = get_mesh(id, i.variation);
				i.update();
			}
		}
	}

	if (field == "maxroll" || field == "colorr" || field == "colorg" || field == "colorb") {
		for (auto& i : doodads) {
			if (i.id == id) {
				i.update();
			}
		}
	}
}

std::shared_ptr<StaticMesh> Doodads::get_mesh(std::string id, int variation) {
	std::string full_id = id + std::to_string(variation);
	if (id_to_mesh.contains(full_id)) {
		return id_to_mesh[full_id];
	}

	fs::path mesh_path;
	std::string variations;
	std::string replaceable_id;
	fs::path texture_name;

	if (doodads_slk.row_headers.contains(id)) {
		// Is doodad
		mesh_path = doodads_slk.data("file", id);
		variations = doodads_slk.data("numvar", id);
	} else {
		mesh_path = destructibles_slk.data("file", id);
		variations = destructibles_slk.data("numvar", id);

		replaceable_id = destructibles_slk.data("texid", id);
		texture_name = destructibles_slk.data("texfile", id);
		texture_name.replace_extension("");
	}

	const std::string stem = mesh_path.stem().string();
	mesh_path.replace_filename(stem + (variations == "1" ? "" : std::to_string(variation)));
	mesh_path.replace_extension(".mdx");

	// Use base model when variation doesn't exist
	if (!hierarchy.file_exists(mesh_path)) {
		mesh_path.remove_filename() /= stem + ".mdx";
	}

	mesh_path = fs::path(string_replaced(mesh_path.string(), "\\", "/"));

	// Mesh doesnt exist at all
	if (!hierarchy.file_exists(mesh_path)) {
		std::cout << "Invalid model file for " << id << " With file path: " << mesh_path << "\n";
		id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>("Objects/Invalidmodel/Invalidmodel.mdx"));
		return id_to_mesh[full_id];
	}

	// Switch around the texture in the replaceable_id table so the mesh loader will pick the correct texture
	std::string replaceable_texture;

	bool replace_texture = is_number(replaceable_id) && texture_name != "_";

	if (replace_texture) {
		replaceable_texture = mdx::replacable_id_to_texture[std::stoi(replaceable_id)];
		mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = texture_name.string() + (hierarchy.hd ? "_diffuse.dds" : ".dds");
	}

	id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>(mesh_path, replace_texture ? texture_name.string() : ""));

	// Switch it back
	if (replace_texture) {
		mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = replaceable_texture;
	}

	return id_to_mesh[full_id];
}

void DoodadAddAction::undo() {
	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadAddAction::redo() {
	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadDeleteAction::undo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadDeleteAction::redo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadStateAction::undo() {
	QRectF update_pathing_area;
	for (auto& i : old_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
					update_pathing_area = { j.position.x, j.position.y, 1.f, 1.f };
				}
				update_pathing_area |= { j.position.x, j.position.y, 1.f, 1.f };
				update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };

				j = i;
			}
		}
	}
	map->doodads.update_doodad_pathing(update_pathing_area);
}

void DoodadStateAction::redo() {
	QRectF update_pathing_area;
	for (auto& i : new_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
					update_pathing_area = { j.position.x, j.position.y, 1.f, 1.f };
				}
				update_pathing_area |= { j.position.x, j.position.y, 1.f, 1.f };
				update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };

				j = i;
			}
		}
	}
	map->doodads.update_doodad_pathing(update_pathing_area);
}
