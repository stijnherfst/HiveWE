#include "stdafx.h"

bool Doodads::load(BinaryReader& reader, Terrain& terrain) {
	std::string magic_number = reader.readString(4);
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
		doodads[i].id = reader.readString(4);
		doodads[i].variation = reader.read<uint32_t>();
		doodads[i].position = reader.read<glm::vec3>() - glm::vec3(terrain.offset, 0);
		doodads[i].angle = reader.read<float>();
		doodads[i].scale = reader.read<glm::vec3>();
		
		uint8_t flag = reader.read<uint8_t>();
		if (flag & 0) {
			doodads[i].state = TreeState::invisible_non_solid;
		} else if (flag & 1) {
			doodads[i].state = TreeState::visible_non_solid;
		} else {
			doodads[i].state = TreeState::visible_solid;
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
	//	std::string special_doodad_id = reader.readString(4);
	//	glm::ivec3 position = reader.read<glm::ivec3>();
	//	std::cout << "\n";
	//}

	create();
	return true;
}

void Doodads::create() {
	doodads_slk = slk::SLK("Doodads\\Doodads.slk");
	destructibles_slk = slk::SLK("Units\\DestructableData.slk");

	for (auto&& i : doodads) {
		if (id_to_mesh.find(i.id) == id_to_mesh.end()) {
			if (id_to_mesh.find(i.id + std::to_string(i.variation)) != id_to_mesh.end()) {
				continue;
			}

			std::string file_name = doodads_slk.data("file", i.id);
			std::string variations = doodads_slk.data("numVar", i.id);
			std::string texture_name;


			if (file_name == "") {
				file_name = destructibles_slk.data("file", i.id);
				texture_name = destructibles_slk.data("texFile", i.id) + ".blp";
			}

			if (variations == "") {
				variations = destructibles_slk.data("numVar", i.id);
			}

			file_name += (variations == "1" ? "" : std::to_string(i.variation)) + ".mdx";

			doodad_meshes.push_back(resource_manager.load<StaticMesh>(file_name));

			if (texture_name != "_.blp") {
				auto tex = resource_manager.load<Texture>(texture_name);

				//SOIL_save_image("Data/test.png", SOIL_SAVE_TYPE_PNG, tex->width, tex->height, 4, tex->data);
				GLuint texture;
				gl->glCreateTextures(GL_TEXTURE_2D, 1, &texture);
				gl->glTextureStorage2D(texture, 1, GL_RGBA8, tex->width, tex->height);
				gl->glTextureSubImage2D(texture, 0, 0, 0, tex->width, tex->height, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
				gl->glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				gl->glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				gl->glGenerateTextureMipmap(texture);

				doodad_meshes.back()->texture = texture;// SOIL_load_OGL_texture("Data/test.png", 0, SOIL_CREATE_NEW_ID, 0);;
			}

			id_to_mesh.emplace(i.id + std::to_string(i.variation), doodad_meshes.size() - 1);
		}
	}

	shader = resource_manager.load<Shader>({ "Data/Shaders/staticmesh.vs", "Data/Shaders/staticmesh.fs" });
}

void Doodads::render() {
	shader->use();
	glm::mat4 Model;
	glm::mat4 MVP;
	for (auto&& i : doodads) {
		Model = glm::translate(glm::mat4(1.0f), i.position / 128.f);
		Model = glm::scale(Model, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f));
		MVP = camera.projection_view * Model;

		gl->glUniformMatrix4fv(2, 1, GL_FALSE, &MVP[0][0]);
		doodad_meshes[id_to_mesh[i.id]]->render();
	}
}