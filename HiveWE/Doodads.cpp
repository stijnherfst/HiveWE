#include "stdafx.h"

bool Doodads::load(BinaryReader& reader, Terrain& terrain) {
	std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!\n";
		return false;
	}
	uint32_t version = reader.read<uint32_t>();
	if (version != 8) {
		std::cout << "Unknown war3map.doo version. Attempting to load but may crash\n";
	}

	// Subversion
	reader.read<uint32_t>();

	int doodads_count = reader.read<uint32_t>();
	
	doodads.resize(doodads_count);
	for (int i = 0; i < doodads_count; i++) {
		doodads[i].id = reader.read_string(4);
		doodads[i].variation = reader.read<uint32_t>();
		doodads[i].position = reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0);
		doodads[i].angle = reader.read<float>();
		doodads[i].scale = reader.read<glm::vec3>();
		
		uint8_t flag = reader.read<uint8_t>();
		if (flag & 0) {
			doodads[i].state = DoodadState::invisible_non_solid;
		} else if (flag & 1) {
			doodads[i].state = DoodadState::visible_non_solid;
		} else {
			doodads[i].state = DoodadState::visible_solid;
		}

		doodads[i].life = reader.read<uint8_t>();

		int item_table_pointer = reader.read<uint32_t>();
		int item_sets_count = reader.read<uint32_t>();
		for (int j = 0; j < item_sets_count; j++) {
			int set_items = reader.read<uint32_t>();
			reader.position += set_items * 8;
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

	doodads_slk = slk::SLK("Doodads\\Doodads.slk");
	doodads_meta_slk = slk::SLK("Doodads\\DoodadMetaData.slk");
	destructibles_slk = slk::SLK("Units\\DestructableData.slk");
	destructibles_meta_slk = slk::SLK("Units\\DestructableMetaData.slk");

	return true;
}

void Doodads::load_destructible_modifications(BinaryReader& reader) {
	// Version
	reader.read<uint32_t>();

	// Original Table
	uint32_t objects = reader.read<uint32_t>();

	for (size_t i = 0; i < objects; i++) {
		std::string original_id = reader.read_string(4);
		reader.position += 4; // Modified id is always 0 for original table

		uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			std::string modification_id = reader.read_string(4);
			uint32_t type = reader.read<uint32_t>();

			std::string column_header = destructibles_meta_slk.data("field", modification_id);

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
				reader.position += 1;
				break;
			}
			reader.position += 4;
			destructibles_slk.set_shadow_data(column_header, original_id, data);
		}
	}

	objects = reader.read<uint32_t>();

	for (size_t i = 0; i < objects; i++) {
		std::string original_id = reader.read_string(4);
		std::string modified_id = reader.read_string(4);

		destructibles_slk.copy_row(original_id, modified_id);

		uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			std::string modification_id = reader.read_string(4);
			uint32_t type = reader.read<uint32_t>();

			std::string column_header = destructibles_meta_slk.data("field", modification_id);

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
				reader.position += 1;
				break;
			}
			reader.position += 4;
			destructibles_slk.set_shadow_data(column_header, modified_id, data);
		}
	}
}

void Doodads::load_doodad_modifications(BinaryReader& reader) {
	// Version
	reader.read<uint32_t>();

	// Original Table
	uint32_t objects = reader.read<uint32_t>();

	for (size_t i = 0; i < objects; i++) {
		std::string original_id = reader.read_string(4);
		reader.position += 4; // Modified id is always 0 for original table

		uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			std::string modification_id = reader.read_string(4);
			uint32_t type = reader.read<uint32_t>();
			uint32_t variation = reader.read<uint32_t>();
			uint32_t data_pointer = reader.read<uint32_t>();

			std::string column_header = doodads_meta_slk.data("field", modification_id);

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
					reader.position += 1;
					break;
			}
			reader.position += 4;
			doodads_slk.set_shadow_data(column_header, original_id, data);
		}
	}

	objects = reader.read<uint32_t>();

	for (size_t i = 0; i < objects; i++) {
		std::string original_id = reader.read_string(4);
		std::string modified_id = reader.read_string(4);

		doodads_slk.copy_row(original_id, modified_id);

		uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			std::string modification_id = reader.read_string(4);
			uint32_t type = reader.read<uint32_t>();
			uint32_t variation = reader.read<uint32_t>();
			uint32_t data_pointer = reader.read<uint32_t>();

			std::string column_header = doodads_meta_slk.data("field", modification_id);

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
				reader.position += 1;
				break;
			}
			reader.position += 4;
			doodads_slk.set_shadow_data(column_header, modified_id, data);
		}
	}
}

void Doodads::create() {
	for (auto&& i : doodads) {
		if (id_to_mesh.find(i.id) == id_to_mesh.end()) {
			if (id_to_mesh.find(i.id + std::to_string(i.variation)) != id_to_mesh.end()) {
				continue;
			}

			fs::path mesh_path = doodads_slk.data("file", i.id);
			std::string variations = doodads_slk.data("numVar", i.id);
			std::string replacable_id;
			fs::path texture_name;

			if (mesh_path.empty()) {
				mesh_path = destructibles_slk.data("file", i.id);
				texture_name = destructibles_slk.data("texFile", i.id);
				replacable_id = destructibles_slk.data("texID", i.id);
			}

			if (variations == "") {
				variations = destructibles_slk.data("numVar", i.id);
			}
			std::string full_id = i.id + std::to_string(i.variation);

			if (!mesh_path.has_extension()) {
				mesh_path += (variations == "1" ? "" : std::to_string(i.variation)) + ".mdx";
			} else {
				mesh_path.replace_extension(".mdx");
			}
			
			if (id_to_mesh.find(full_id) == id_to_mesh.end()) {
				id_to_mesh.emplace(full_id, resource_manager.load<StaticMesh>(mesh_path.string()));
			}

			if (!texture_name.empty() && texture_name != "_") {
				if (id_to_mesh[full_id]->textures.size() > 0 && id_to_mesh[full_id]->textures[0]->id == 0) {
					texture_name.replace_extension(".blp");
					id_to_mesh[full_id]->textures[0] = resource_manager.load<GPUTexture>(texture_name.string());
				}
			}
		}
	}

	shader = resource_manager.load<Shader>({ "Data/Shaders/staticmesh.vs", "Data/Shaders/staticmesh.fs" });
}

void Doodads::render() {
	shader->use();
	glm::mat4 model;
	glm::mat4 MVP;
	for (auto&& i : doodads) {
		model = glm::translate(glm::mat4(1.0f), i.position / 128.f);
		model = glm::scale(model, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f) * i.scale);
		model = glm::rotate(model, i.angle, glm::vec3(0, 0, 1));
		MVP = camera.projection_view * model;

		gl->glUniformMatrix4fv(2, 1, GL_FALSE, &MVP[0][0]);
		id_to_mesh[i.id + std::to_string(i.variation)]->render();
	}
}