#include "stdafx.h"

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

	const int doodads_count = reader.read<uint32_t>();

	doodads.resize(doodads_count);
	for (int i = 0; i < doodads_count; i++) {
		doodads[i].id = reader.read_string(4);
		doodads[i].variation = reader.read<uint32_t>();
		doodads[i].position = reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0);
		doodads[i].angle = reader.read<float>();
		doodads[i].scale = reader.read<glm::vec3>();

		const uint8_t flag = reader.read<uint8_t>();
		if (flag & 0) {
			doodads[i].state = DoodadState::invisible_non_solid;
		} else if (flag & 1) {
			doodads[i].state = DoodadState::visible_non_solid;
		} else {
			doodads[i].state = DoodadState::visible_solid;
		}

		doodads[i].life = reader.read<uint8_t>();

		if (version >= 8) {
			int item_table_pointer = reader.read<uint32_t>();
			const int item_sets_count = reader.read<uint32_t>();
			for (int j = 0; j < item_sets_count; j++) {
				const int set_items = reader.read<uint32_t>();
				reader.position += set_items * 8;
			}
		}

		int world_editor_id = reader.read<uint32_t>();
	}

	//int special_format_version = reader.read<uint32_t>();
	//int special_doodads_count = reader.read<uint32_t>();

	//for (int i = 0; i < special_doodads_count; i++) {
	//	std::string special_doodad_id = reader.read_string(4);
	//	glm::ivec3 position = reader.read<glm::ivec3>();
	//	std::cout << "\n";
	//}

	doodads_slk = slk::SLK("Doodads/Doodads.slk");
	doodads_meta_slk = slk::SLK("Doodads/DoodadMetaData.slk");
	destructibles_slk = slk::SLK("Units/DestructableData.slk");
	destructibles_meta_slk = slk::SLK("Units/DestructableMetaData.slk");

	return true;
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

void Doodads::create() {
	for (auto&& i : doodads) {
		// Do this properly somewhere else
		i.matrix = glm::translate(i.matrix, i.position / 128.f);
		i.matrix = glm::scale(i.matrix, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f) * i.scale);
		i.matrix = glm::rotate(i.matrix, i.angle, glm::vec3(0, 0, 1));


		std::string full_id = i.id + std::to_string(i.variation);
		if (id_to_mesh.find(full_id) != id_to_mesh.end()) {
			continue;
		}

		if (i.id == "D00E") {
			std::cout << "\n";
		}

		fs::path mesh_path;
		std::string variations;
		std::string replaceable_id;
		fs::path texture_name;

		if (doodads_slk.header_to_row.find(i.id) != doodads_slk.header_to_row.end()) {
			// Is doodad
			mesh_path = doodads_slk.data("file", i.id);
			variations = doodads_slk.data("numVar", i.id);
		} else {
			// Is destructible
			mesh_path = destructibles_slk.data("file", i.id);
			variations = destructibles_slk.data("numVar", i.id);

			replaceable_id = destructibles_slk.data("texID", i.id);
			texture_name = destructibles_slk.data("texFile", i.id);
			texture_name.replace_extension(".blp");
		}

		const std::string stem = mesh_path.stem().string();
		mesh_path.replace_filename(stem + (variations == "1" ? "" : std::to_string(i.variation)));
		mesh_path.replace_extension(".mdx");

		// Use base model when variation doesn't exist
		if (!hierarchy.file_exists(mesh_path)) {
			mesh_path.remove_filename() /= stem + ".mdx";
		}

		// Mesh doesnt exist at all
		if (!hierarchy.file_exists(mesh_path)) {
			std::cout << "Invalid model file for " << i.id << " With file path: " << mesh_path << "\n";
			id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>("Objects/Invalidmodel/Invalidmodel.mdx"));
			continue;
		}

		// Switch around the texture in the replaceable_id table
		std::string replaceable_texture;
		if (is_number(replaceable_id) && texture_name != "_.blp") {
			replaceable_texture = mdx::replacable_id_to_texture[std::stoi(replaceable_id)];
			mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = texture_name.string();
		}

		id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>(mesh_path.string()));

		// Switch it back
		if (is_number(replaceable_id) && texture_name != "_.blp") {
			mdx::replacable_id_to_texture[std::stoi(replaceable_id)] = replaceable_texture;
		}
	}
}

void Doodads::render() {
	for (auto&& i : doodads) {
		id_to_mesh[i.id + std::to_string(i.variation)]->render_queue(i.matrix);
	}
}