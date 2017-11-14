#include "stdafx.h"

int Terrain::get_tile_variation(Corner& tile_corner) {
	bool extended = ground_textures[tile_corner.ground_texture].get()->width == ground_textures[tile_corner.ground_texture].get()->height * 2;
	if (extended) {
		if (tile_corner.ground_variation <= 15) {
			return 16 + tile_corner.ground_variation;
		} else if (tile_corner.ground_variation == 16) {
			return 15;
		} else {
			return 0;
		}
	} else {
		if (tile_corner.ground_variation == 0) {
			return 0;
		} else {
			return 15;
		}
	}
}

std::vector<std::tuple<int, int>> Terrain::get_texture_variations(Corner& topL, Corner& topR, Corner& bottomL, Corner& bottomR) {
	std::vector<std::tuple<int, int>> tileVariations;
	
	auto comp = [&](Corner l, Corner r) { return (l.blight ? blight_texture : l.ground_texture) < (r.blight ? blight_texture : r.ground_texture); };
	std::set<Corner, decltype(comp)> set({ topL, topR, bottomL, bottomR }, comp);

	Corner first = *set.begin();
	tileVariations.push_back({ get_tile_variation(first), first.blight ? blight_texture : first.ground_texture });
	set.erase(set.begin());

	std::bitset<4> index;
	for (auto&& corner : set) {
		int texture = corner.blight ? blight_texture : corner.ground_texture;
		index[0] = (bottomR.blight ? blight_texture : bottomR.ground_texture) == texture;
		index[1] = (bottomL.blight ? blight_texture : bottomL.ground_texture) == texture;
		index[2] = (topR.blight ? blight_texture : topR.ground_texture) == texture;
		index[3] = (topL.blight ? blight_texture : topL.ground_texture) == texture;

		tileVariations.push_back({ index.to_ulong(), texture });
	}
	return tileVariations;
}

void Terrain::create() {
	vertices.reserve(width * height * 4);
	uvs.reserve(width * height * 4);
	indices.reserve((width - 1) * (height - 1) * 2);

	for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			Corner& bottomLeft = corners[j * width + i];
			Corner& bottomRight = corners[j * width + (i + 1)];
			Corner& topLeft = corners[(j + 1) * width + i];
			Corner& topRight = corners[(j + 1) * width + (i + 1)];

			// Cliffs
			if (bottomLeft.cliff) {
				// Cliff model path
				int base = std::min({ bottomLeft.layer_height, bottomRight.layer_height, topLeft.layer_height, topRight.layer_height });
				std::string file_name = ""s + (char)('A' + bottomLeft.layer_height - base)
					+ (char)('A' + topLeft.layer_height - base)
					+ (char)('A' + topRight.layer_height - base)
					+ (char)('A' + bottomRight.layer_height - base);

				if (file_name == "AAAA") {
					continue;
				}

				// Check if it needs to be loaded
				if (path_to_cliff.find(file_name) == path_to_cliff.end()) {
					cliff_meshes.push_back(resource_manager.load<StaticMesh>("Doodads\\Terrain\\Cliffs\\Cliffs" + file_name + "0.mdx"));
					path_to_cliff.emplace(file_name, (int)cliff_meshes.size() - 1);
				}

				cliffs.push_back({ i + 1, j, path_to_cliff[file_name] });

				continue;
			}

			// Ground tiles
			auto variations = get_texture_variations(bottomLeft, bottomRight, topLeft, topRight); // TODO Bottom and top reversed, fix
			for (auto&& [variation, texture] : variations) {
				vertices.push_back({ i + 1, j + 1,	topRight.height() });
				vertices.push_back({ i,		j + 1,	topLeft.height() });
				vertices.push_back({ i,		j,		bottomLeft.height() });
				vertices.push_back({ i + 1, j,		bottomRight.height() });

				uvs.push_back({ 1, 1, texture * 32 + variation });
				uvs.push_back({ 0, 1, texture * 32 + variation });
				uvs.push_back({ 0, 0, texture * 32 + variation });
				uvs.push_back({ 1, 0, texture * 32 + variation });

				unsigned int index = vertices.size() - 4;
				indices.push_back({ index + 0, index + 1, index + 2 });
				indices.push_back({ index + 0, index + 2, index + 3 });
			}
		}
	}

	gl->glGenBuffers(1, &vertexBuffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &uvBuffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), uvs.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &indexBuffer);
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int) * 3, indices.data(), GL_STATIC_DRAW);

	// Ground textures
	gl->glGenTextures(1, &ground_texture_array);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, ground_texture_array);
	gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, variation_width, variation_height, ground_textures.size() * 32);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int sub = 0;
	for (auto&& i : ground_textures) {
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, i->width);
		for (size_t y = 0; y < 4; y++) {
			for (size_t x = 0; x < 4; x++) {
				int sub_image = sub * 32 + y * 4 + x;
				gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, sub_image, variation_width, variation_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, i->data + (y * variation_height * i->width + x * variation_width) * 4);

				// If extended
				if (i->width == i->height * 2) {
					gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, sub_image + 16, variation_width, variation_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, i->data + (y * variation_height * i->width + (x + 4) * variation_width) * 4);
				}
			}
		}
		sub += 1;
	}
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	gl->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	// Cliff textures
	cliff_texture_list.resize(cliff_textures.size());
	gl->glGenTextures(cliff_texture_list.size(), cliff_texture_list.data());
	for (size_t i = 0; i < cliff_textures.size(); i++) {
		gl->glBindTexture(GL_TEXTURE_2D, cliff_texture_list[i]);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cliff_textures[i]->width, cliff_textures[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cliff_textures[i]->data);
		gl->glGenerateMipmap(GL_TEXTURE_2D);
	}

	for (auto&& i : cliff_meshes) {
		i->texture = cliff_texture_list[0];
	}
}

bool Terrain::load(std::vector<uint8_t> data) {
	BinaryReader reader(data);

	std::string magic_number = reader.readString(4);
	if (magic_number != "W3E!") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
		return false;
	}
	uint32_t version = reader.read<uint32_t>();

	char tileset = reader.read<char>();
	bool custom_tileset = reader.read<uint32_t>() == 1 ? true : false; // 0 for not default, 1 for custom

	uint32_t tileset_textures = reader.read<uint32_t>();
	if (tileset_textures > 16) {
		std::cout << "Invalid war3map.w3e file: More than 16 textures" << std::endl;
		return false;
	}
	for (size_t i = 0; i < tileset_textures; i++) {
		tileset_ids.push_back(reader.readString(4));
	}

	int cliffset_textures = reader.read<uint32_t>();
	for (size_t i = 0; i < cliffset_textures; i++) {
		cliffset_ids.push_back(reader.readString(4));
	}

	width = reader.read<uint32_t>();
	height = reader.read<uint32_t>();

	offset_x = reader.read<float>();
	offset_y = reader.read<float>();

	// Parse all tilepoints
	Corner corner;
	for (size_t j = 0; j < height; j++) {
		for (size_t i = 0; i < width; i++) {
			corner.ground_height = reader.read<int16_t>();

			int16_t water_and_edge = reader.read<int16_t>();
			corner.water_height = water_and_edge & 0x3FFF;
			corner.map_edge = water_and_edge & 0xC000;

			int8_t texture_and_flags = reader.read<int8_t>();
			corner.ground_texture = texture_and_flags & 0x0F;

			int8_t flags = texture_and_flags & 0xF0;
			corner.ramp = flags & 0X0010;
			corner.blight = flags & 0x0020;
			corner.water = flags & 0x0040;
			corner.boundary = flags & 0x4000;

			int8_t variation = reader.read<int8_t>();
			corner.ground_variation = variation & 31;
			corner.cliff_variation = (variation & 224) >> 5;

			int8_t misc = reader.read<int8_t>();
			corner.cliff_texture = (misc & 0xF0) >> 4;
			corner.layer_height = misc & 0x0F;

			corners.push_back(corner);
		}
	}

	// Determine if cliff
	for (size_t i = 0; i < width; i++) {
		for (size_t j = 0; j < height; j++) {
			Corner& corner = corners[j * width + i];
			corner.cliff = false;

			if (i + 1 < width - 1) {
				if (corners[j * width + (i + 1)].layer_height != corner.layer_height) {
					corner.cliff = true;
				}
			}

			if (i + 1 < width - 1 && j + 1 < height - 1) {
				if (corners[(j + 1) * width + (i + 1)].layer_height != corner.layer_height) {
					corner.cliff = true;
				}
			}

			if (j + 1 < height - 1) {
				if (corners[(j + 1) * width + i].layer_height != corner.layer_height) {
					corner.cliff = true;
				}
			}
		}
	}
	// Done parsing

	hierarchy.init(tileset);

	// Ground Textures
	slk::SLK slk("TerrainArt\\Terrain.slk");
	for (size_t i = 0; i < tileset_ids.size(); i++) {
		for (size_t j = 0; j < slk.data[0].size(); j++) {
			if (slk.data[0][j] == tileset_ids[i]) {
				ground_textures.push_back(resource_manager.load<Texture>(slk.data[2][j] + "\\" + slk.data[3][j] + ".blp"));
			}
		}
	}
	ground_textures.push_back(resource_manager.load<Texture>("TerrainArt\\Blight\\Ashen_Blight.blp"));
	blight_texture = ground_textures.size() - 1;

	// Cliff Textures
	slk::SLK cliff_slk("TerrainArt\\CliffTypes.slk");
	for (auto&& cliff_id : cliffset_ids) {
		for (size_t j = 0; j < cliff_slk.data[0].size(); j++) {
			if (cliff_slk.data[0][j] == cliff_id) {
				cliff_textures.push_back(resource_manager.load<Texture>(cliff_slk.data[3][j] + "\\" + cliff_slk.data[4][j] + ".blp"));
			}
		}
	}

	ground_shader = resource_manager.load<Shader>({ "Data/Shaders/terrain.vs", "Data/Shaders/terrain.fs" });
	cliff_shader = resource_manager.load<Shader>({ "Data/Shaders/staticmesh.vs", "Data/Shaders/staticmesh.fs" });

	create();

	return true;
}

void Terrain::render() {
	// Render tiles
	ground_shader->use();

	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::translate(Model, glm::vec3(0, 0, 0));
	glm::mat4 MVP = camera.projection * camera.view * Model;

	gl->glUniformMatrix4fv(gl->glGetUniformLocation(ground_shader->program, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	gl->glActiveTexture(GL_TEXTURE0);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, ground_texture_array);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT, NULL);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);

	cliff_shader->use();
	// Render cliffs
	for (auto&& i : cliffs) {
		int min = std::min({ corners[i.y * width + i.x].height() , corners[i.y * width + i.x - 1].height(), corners[(i.y + 1) * width + i.x].height(), corners[(i.y + 1) * width + i.x - 1].height() });
		Model = glm::translate(glm::mat4(1.0f), glm::vec3(i.x, i.y, min));
		Model = glm::scale(Model, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f));
		MVP = camera.projection * camera.view * Model;
		gl->glUniformMatrix4fv(gl->glGetUniformLocation(cliff_shader->program, "MVP"), 1, GL_FALSE, &MVP[0][0]);

		cliff_meshes[i.z]->render();
	}
}