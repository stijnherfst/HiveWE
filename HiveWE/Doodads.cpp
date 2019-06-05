#include "stdafx.h"

int Doodad::auto_increment;

void Doodad::update() {
	glm::vec3 base_scale = glm::vec3(1.f);

	if (doodads_slk.row_header_exists(id)) {
		base_scale = glm::vec3(doodads_slk.data<float>("defScale", id));
	}

	matrix = glm::translate(glm::mat4(1.f), position);
	matrix = glm::scale(matrix, (base_scale - 1.f + scale) / 128.f);
	matrix = glm::rotate(matrix, angle, glm::vec3(0, 0, 1));
}

bool Doodads::load(BinaryReader& reader, Terrain& terrain) {
	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3do\n";
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
		i.position = (reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>();
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
		writer.write<glm::ivec2>(glm::ivec2(i.position.x, i.position.y) - 2);
	}

	hierarchy.map_file_write("war3map.doo", writer.buffer);
}


void Doodads::load_destructible_modifications(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown destructible modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, destructibles_slk, destructibles_meta_slk, false);
	load_modification_table(reader, destructibles_slk, destructibles_meta_slk, true);
}

void Doodads::load_doodad_modifications(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown doodad modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, doodads_slk, doodads_meta_slk, false, true);
	load_modification_table(reader, doodads_slk, doodads_meta_slk, true, true);
}

//void Doodads::update_area(const QRect& area) {
//	auto undo = std::make_unique<DoodadStateAction>();
//
//	// ToDo optimize with parallel for?
//	for (auto&& i : doodads) {
//		if (area.contains(i.position.x, i.position.y)) {
//			undo->old_doodads.push_back(i);
//			i.position.z = map->terrain.corners[i.position.x][i.position.y].final_ground_height();
//			i.update();
//			undo->new_doodads.push_back(i);
//		}
//	}
//	map->terrain_undo.add_undo_action(std::move(undo));
//}

void Doodads::create() {
	for (auto&& i : doodads) {
		i.update();
		i.mesh = get_mesh(i.id, i.variation);

		// Get pathing map
		const bool is_doodad = doodads_slk.row_header_exists(i.id);
		const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

		std::string pathing_texture_path = slk.data("pathTex", i.id);
		if (hierarchy.file_exists(pathing_texture_path)) {
			i.pathing = resource_manager.load<Texture>(pathing_texture_path);
		}
	}

	for (auto&& i : special_doodads) {
		float rotation = doodads_slk.data<int>("fixedRot", i.id) / 360.f * 2.f * glm::pi<float>();
		i.matrix = glm::translate(i.matrix, i.position);
		i.matrix = glm::scale(i.matrix, { 1 / 128.f, 1 / 128.f, 1 / 128.f });
		i.matrix = glm::rotate(i.matrix, rotation, glm::vec3(0, 0, 1));
		i.mesh = get_mesh(i.id, i.variation);
	}
}

void Doodads::render() {
	for (auto&& i : doodads) {
		i.mesh->render_queue(i.matrix);	
	}
	for (auto&& i : special_doodads) {
		i.mesh->render_queue(i.matrix);
	}
}

Doodad& Doodads::add_doodad(std::string id, int variation, glm::vec3 position) {
	Doodad doodad;
	doodad.id = id;
	doodad.variation = variation;
	doodad.mesh = get_mesh(id, variation);
	doodad.position = position;
	doodad.scale = { 1, 1, 1 };
	doodad.angle = 0;

	const bool is_doodad = doodads_slk.row_header_exists(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
	std::string pathing_texture_path = slk.data("pathTex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		doodad.pathing = resource_manager.load<Texture>(pathing_texture_path);
	}

	doodad.update();

	doodads.push_back(doodad);
	return doodads.back();
}

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
	}), doodads.end());
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
		map->pathing_map.blit_pathing_texture(i->position, 0, i->pathing);
	}
	map->pathing_map.upload_dynamic_pathing();
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

	if (doodads_slk.row_header_exists(id)) {
		// Is doodad
		mesh_path = doodads_slk.data("file", id);
		variations = doodads_slk.data("numVar", id);
	} else {
		// Is destructible
		mesh_path = destructibles_slk.data("file", id);
		variations = destructibles_slk.data("numVar", id);

		replaceable_id = destructibles_slk.data("texID", id);
		texture_name = destructibles_slk.data("texFile", id);
		texture_name.replace_extension(".blp");
	}

	const std::string stem = mesh_path.stem().string();
	mesh_path.replace_filename(stem + (variations == "1" ? "" : std::to_string(variation)));
	mesh_path.replace_extension(".mdx");

	// Use base model when variation doesn't exist
	if (!hierarchy.file_exists(mesh_path)) {
		mesh_path.remove_filename() /= stem + ".mdx";
	}

	// Mesh doesnt exist at all
	if (!hierarchy.file_exists(mesh_path)) {
		std::cout << "Invalid model file for " << id << " With file path: " << mesh_path << "\n";
		id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>("Objects/Invalidmodel/Invalidmodel.mdx"));
		return id_to_mesh[full_id];
	}

	// Switch around the texture in the replaceable_id table so the mesh loader will pick the correct texture
	std::string replaceable_texture;
	if (is_number(replaceable_id) && texture_name != "_.blp") {
		replaceable_texture = mdx::replacable_id_to_texture[std::stoi(replaceable_id)];
		mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = texture_name.string();
	}

	id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>(mesh_path));

	// Switch it back
	if (is_number(replaceable_id) && texture_name != "_.blp") {
		mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = replaceable_texture;
	}

	return id_to_mesh[full_id];
}

void DoodadAddAction::undo() {
	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());

	QRectF update_pathing_area;
	for (const auto& i : doodads) {

		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	map->doodads.update_doodad_pathing(update_pathing_area);
}

void DoodadAddAction::redo() {
	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());

	QRectF update_pathing_area;
	for (const auto& i : doodads) {

		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	// Update pathing
	map->doodads.update_doodad_pathing(update_pathing_area);
}

void DoodadDeleteAction::undo() {
	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());

	QRectF update_pathing_area;
	for (const auto& i : doodads) {

		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	// Update pathing
	map->doodads.update_doodad_pathing(update_pathing_area);
}
void DoodadDeleteAction::redo() {
	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());

	QRectF update_pathing_area;
	for (const auto& i : doodads) {

		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	// Update pathing
	map->doodads.update_doodad_pathing(update_pathing_area);
}

void DoodadStateAction::undo() {
	for (auto& i : old_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				j = i;
			}
		}
	}
}

void DoodadStateAction::redo() {
	for (auto& i : new_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				j = i;
			}
		}
	}
}
