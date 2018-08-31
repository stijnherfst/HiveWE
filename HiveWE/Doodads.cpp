#include "stdafx.h"

int Doodad::auto_increment;

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
	reader.read<uint32_t>();

	doodads.resize(reader.read<uint32_t>());
	for (auto&& i : doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = (reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>() / 128.f;
		i.state = static_cast<Doodad::State>(reader.read<uint8_t>());
		i.life = reader.read<uint8_t>();

		if (version >= 8) {
			i.item_table_pointer = reader.read<uint32_t>();
			i.item_sets.resize(reader.read<uint32_t>());
			for (auto&& j : i.item_sets) {
				j.items.resize(reader.read<uint32_t>());
				for (auto&& [id, chance] : j.items) {
					id = reader.read_string(4);
					chance = reader.read<uint32_t>();
				}
			}
		}

		i.world_editor_id = reader.read<uint32_t>();
	}

	// Terrain Doodads
	const int special_format_version = reader.read<uint32_t>();

	special_doodads.resize(reader.read<uint32_t>());
	for (auto&& i : special_doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = glm::ivec3(reader.read<glm::ivec2>(), 0);
	}

	doodads_slk = slk::SLK("Doodads/Doodads.slk");
	doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");
	doodads_meta_slk = slk::SLK("Doodads/DoodadMetaData.slk");
	destructibles_slk = slk::SLK("Units/DestructableData.slk");
	destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructibles_slk.substitute(world_edit_game_strings, "WorldEditStrings");
	destructibles_meta_slk = slk::SLK("Units/DestructableMetaData.slk");

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
		writer.write<glm::vec3>(i.position * 128.f + glm::vec3(map.terrain.offset, 0));
		writer.write<float>(i.angle);
		writer.write<glm::vec3>(i.scale * 128.f);

		writer.write<uint8_t>(static_cast<int>(i.state));
		writer.write<uint8_t>(i.life);

		writer.write<int32_t>(i.item_table_pointer);
		writer.write<uint32_t>(i.item_sets.size());
		for (auto&& j : i.item_sets) {
			writer.write<uint32_t>(j.items.size());
			for (auto&& [id, chance] : j.items) {
				writer.write_string(id);
				writer.write<uint32_t>(chance);
			}
		}

		writer.write<uint32_t>(i.world_editor_id);
	}

	writer.write<uint32_t>(write_special_version);

	writer.write<uint32_t>(special_doodads.size());
	for (auto&& i : special_doodads) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::ivec2>(glm::ivec2(i.position.x, i.position.y) - 2);
	}

	HANDLE handle;
	bool success = SFileCreateFile(hierarchy.map.handle, "war3map.doo", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
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

void Doodads::update_area(const QRect& area) {
	// ToDo optimize with parallel for?
	for (auto&& i : doodads) {
		if (area.contains(i.position.x, i.position.y)) {
			i.position.z = map.terrain.corner_height(i.position.x, i.position.y);
			i.matrix = glm::translate(glm::mat4(1.f), i.position);
			i.matrix = glm::scale(i.matrix, i.scale);
			i.matrix = glm::rotate(i.matrix, i.angle, glm::vec3(0, 0, 1));
		}
	}
}

void Doodads::create() {
	for (auto&& i : doodads) {
		i.matrix = glm::translate(i.matrix, i.position);
		i.matrix = glm::scale(i.matrix, i.scale);
		i.matrix = glm::rotate(i.matrix, i.angle, glm::vec3(0, 0, 1));
		i.mesh = get_mesh(i.id, i.variation);
	}

	for (auto&& i : special_doodads) {
		float rotation = std::stoi(doodads_slk.data("fixedRot", i.id)) / 360.f * 2.f * glm::pi<float>();
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

Doodad& Doodads::add_doodad(std::string id, glm::vec2 position) {
	Doodad doodad;
	doodad.id = id;
	doodad.mesh = get_mesh(id, 0);
	doodad.position = glm::vec3(position, 0);
	doodad.matrix = glm::translate(doodad.matrix, doodad.position);
	doodad.matrix = glm::scale(doodad.matrix, { 1 / 128.f, 1 / 128.f, 1 / 128.f });

	doodads.push_back(doodad);
	return doodads.back();
}

void Doodads::remove_doodad(Doodad* doodad) {
	auto iterator = doodads.begin() + std::distance(doodads.data(), doodad);
	doodads.erase(iterator);
}

std::vector<Doodad*> Doodads::query_area(QRectF area) {
	std::vector<Doodad*> result;

	for (auto&& i : doodads) {
		if (area.contains(i.position.x, i.position.y)) {
			result.push_back(&i);
		}
	}
	return result;
}

void Doodads::remove_doodads(const std::vector<Doodad*> list) {
	doodads.erase(std::remove_if(doodads.begin(), doodads.end(), [&](Doodad& doodad) {
		return std::find(list.begin(), list.end(), &doodad) != list.end();
	}), doodads.end());
}

std::shared_ptr<StaticMesh> Doodads::get_mesh(std::string id, int variation) {
	std::string full_id = id + std::to_string(variation);
	if (id_to_mesh.find(full_id) != id_to_mesh.end()) {
		return id_to_mesh[full_id];
	}

	fs::path mesh_path;
	std::string variations;
	std::string replaceable_id;
	fs::path texture_name;

	if (doodads_slk.header_to_row.find(id) != doodads_slk.header_to_row.end()) {
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