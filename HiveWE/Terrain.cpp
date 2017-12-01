#include "stdafx.h"

float Terrain::corner_height(Corner corner) const {
	return (corner.ground_height - 0x2000 + (corner.layer_height - 2) * 0x0200) / 512.0;
}

float Terrain::corner_water_height(Corner corner) const {
	return (corner.water_height - 0x2000 + (height_offset * 512)) / 512.0;
}

float Terrain::corner_cliff_height(Corner corner) const {
	return ((corner.layer_height - 2) * 0x0200 + corner.ground_height - 0x2000) / 512.0;
}

int Terrain::real_tile_texture(int x, int y) {
	for (int i = -1; i < 1; i++) {
		for (int j = -1; j < 1; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) {
				if (corners[(y + j) * width + (x + i)].cliff) {
					int texture = corners[(y + j) * width + (x + i)].cliff_texture;
					// Number 15 seems to be both grass and dirt ramp? How to distinguish?
					if (texture == 15) {
						texture -= 14;
					}
					return cliff_to_ground_texture[texture];
				}
			}
		}
	}
	if (corners[y * width + x].blight) {
		 return blight_texture;
	}
		
	return corners[y * width + x].ground_texture;
}

int Terrain::get_tile_variation(const Corner& tile_corner) {
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

std::vector<std::tuple<int, int>> Terrain::get_texture_variations(int x, int y) {
	std::vector<std::tuple<int, int>> tileVariations;

	// Bottom and top reversed
	auto bottomL = std::make_tuple(real_tile_texture(x, y + 1), corners[(y + 1) * width + x]);
	auto bottomR = std::make_tuple(real_tile_texture(x + 1, y + 1), corners[(y + 1) * width + x + 1]);
	auto topL = std::make_tuple(real_tile_texture(x, y), corners[y * width + x]);
	auto topR = std::make_tuple(real_tile_texture(x + 1, y), corners[y * width + x + 1]);
		
	auto comp = [&](std::tuple<int, Corner> l, std::tuple<int, Corner> r) {
		return std::get<0>(l) < std::get<0>(r);
	};

	std::set<std::tuple<int, Corner>, decltype(comp)> set({ topL, topR, bottomL, bottomR }, comp);

	auto [texture, corner] = *set.begin();
	tileVariations.push_back({ get_tile_variation(corner), texture });
	set.erase(set.begin());

	std::bitset<4> index;
	for (auto [texture, corner] : set) {
		// Bottom and top reversed
		index[0] = real_tile_texture(x + 1, y + 1) == texture;
		index[1] = real_tile_texture(x, y + 1) == texture;
		index[2] = real_tile_texture(x + 1, y) == texture;
		index[3] = real_tile_texture(x, y) == texture;

		tileVariations.push_back({ index.to_ulong(), texture });
	}
	return tileVariations;
}

void Terrain::create() {
	// Reserve the minimum guaranteed amount
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

				// Clamp to within max variations
				file_name += std::to_string(std::clamp(bottomLeft.cliff_variation, 0, cliff_variations[file_name]));

				cliffs.push_back({ i, j, path_to_cliff[file_name] });
			}

			// Ground tiles
			if (!bottomLeft.cliff) {
				auto variations = get_texture_variations(i, j);
				for (auto&&[variation, texture] : variations) {
					vertices.push_back({ i + 1, j + 1,	corner_height(topRight) });
					vertices.push_back({ i,		j + 1,	corner_height(topLeft) });
					vertices.push_back({ i,		j,		corner_height(bottomLeft) });
					vertices.push_back({ i + 1, j,		corner_height(bottomRight) });

					uvs.push_back({ 1, 1, texture * 32 + variation });
					uvs.push_back({ 0, 1, texture * 32 + variation });
					uvs.push_back({ 0, 0, texture * 32 + variation });
					uvs.push_back({ 1, 0, texture * 32 + variation });

					unsigned int index = vertices.size() - 4;
					indices.push_back({ index + 0, index + 3, index + 1 });
					indices.push_back({ index + 1, index + 3, index + 2 });
				}
			}

			// Water
			if (bottomLeft.water || bottomRight.water || topLeft.water || topRight.water) {
				water_vertices.push_back({ i + 1,	j + 1,	corner_water_height(bottomLeft) });
				water_vertices.push_back({ i,		j + 1,	corner_water_height(bottomLeft) });
				water_vertices.push_back({ i,		j,		corner_water_height(bottomLeft) });
				water_vertices.push_back({ i + 1,	j,		corner_water_height(bottomLeft) });

				water_uvs.push_back({ 1, 1 });
				water_uvs.push_back({ 0, 1 });
				water_uvs.push_back({ 0, 0 });
				water_uvs.push_back({ 1, 0 });

				// Calculate water colour based on distance to the terrain
				glm::vec4 color;
				for (auto&& corner : { topRight, topLeft, bottomLeft, bottomRight }) {
					float value = std::clamp(corner_water_height(corner) - corner_height(corner), 0.f, 1.f);
					if (value <= deeplevel) {
						value = std::max(0.f, value - min_depth) / (deeplevel - min_depth);
						color = shallow_color_min * (1.f - value) + shallow_color_max * value;
					} else {
						value = std::clamp(value - deeplevel, 0.f, maxdepth - deeplevel) / (maxdepth - deeplevel);
						color = deep_color_min * (1.f - value) + deep_color_max * value;
					}
					water_colors.push_back(color / glm::vec4(255, 255, 255, 255));
				}

				unsigned int index = water_vertices.size() - 4;
				water_indices.push_back({ index + 0, index + 3, index + 1 });
				water_indices.push_back({ index + 1, index + 3, index + 2 });
			}
		}
	}

	// Ground
	gl->glGenBuffers(1, &vertex_buffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &uv_buffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), uvs.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &index_buffer);
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int) * 3, indices.data(), GL_STATIC_DRAW);

	// Water
	gl->glGenBuffers(1, &water_vertex_buffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_vertex_buffer);
	gl->glBufferData(GL_ARRAY_BUFFER, water_vertices.size() * sizeof(glm::vec3), water_vertices.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &water_uv_buffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_uv_buffer);
	gl->glBufferData(GL_ARRAY_BUFFER, water_uvs.size() * sizeof(glm::vec2), water_uvs.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &water_color_buffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_color_buffer);
	gl->glBufferData(GL_ARRAY_BUFFER, water_colors.size() * sizeof(glm::vec4), water_colors.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &water_index_buffer);
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, water_index_buffer);
	gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, water_indices.size() * sizeof(unsigned int) * 3, water_indices.data(), GL_STATIC_DRAW);

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

	// Water textures
	gl->glGenTextures(1, &water_texture_array);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, water_texture_array);
	gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 128, 128, water_textures_nr);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (size_t i = 0; i < water_textures.size(); i++) {
		if (water_textures[i]->width > 128 || water_textures[i]->height > 128) {
			std::cout << "Odd water texture size detected of " << water_textures[i]->width << " wide and " << water_textures[i]->height << " high\n";
			continue;
		}
		gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 128, 128, 1, GL_RGBA, GL_UNSIGNED_BYTE, water_textures[i]->data);
	}
	gl->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
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
	for (size_t i = 0; i < width - 1; i++) {
		for (size_t j = 0; j < height - 1; j++) {
			Corner& bottomLeft = corners[j * width + i];
			Corner& bottomRight = corners[j * width + (i + 1)];
			Corner& topLeft = corners[(j + 1) * width + i];
			Corner& topRight = corners[(j + 1) * width + (i + 1)];

			if (bottomLeft.layer_height != bottomRight.layer_height
				|| bottomLeft.layer_height != topLeft.layer_height
				|| bottomLeft.layer_height != topRight.layer_height) {

				bottomLeft.cliff = true;
			}
		}
	}
	// Done parsing

	hierarchy.init(tileset);

	// Ground Textures
	slk::SLK slk("TerrainArt\\Terrain.slk");
	for (auto&& tile_id : tileset_ids) {
		ground_textures.push_back(resource_manager.load<Texture>(slk.data("dir", tile_id) + "\\" + slk.data("file", tile_id) + ".blp"));
		ground_texture_to_id.emplace(tile_id, ground_textures.size() - 1);
	}
	ground_textures.push_back(resource_manager.load<Texture>("TerrainArt\\Blight\\Ashen_Blight.blp"));
	blight_texture = ground_textures.size() - 1;

	// Cliff Textures
	slk::SLK cliff_slk("TerrainArt\\CliffTypes.slk");
	for (auto&& cliff_id : cliffset_ids) {
		cliff_textures.push_back(resource_manager.load<Texture>(cliff_slk.data("texDir", cliff_id) + "\\" + cliff_slk.data("texFile", cliff_id) + ".blp"));
		cliff_to_ground_texture.push_back(ground_texture_to_id[cliff_slk.data("groundTile", cliff_id)]);
	}

	// Water Textures and Colours
	slk::SLK water_slk("TerrainArt\\Water.slk");

	height_offset = std::stof(water_slk.data("height", tileset + "Sha"s));
	water_textures_nr = std::stoi(water_slk.data("numTex", tileset + "Sha"s));
	animation_rate = std::stoi(water_slk.data("texRate", tileset + "Sha"s));

	std::string file_name = water_slk.data("texFile", tileset + "Sha"s);
	for (size_t i = 0; i < water_textures_nr; i++) {
		water_textures.push_back(resource_manager.load<Texture>(file_name + (i < 10 ? "0" + std::to_string(i) : std::to_string(i)) + ".blp"));
	}

	int red = std::stoi(water_slk.data("Smin_R", tileset + "Sha"s));
	int green = std::stoi(water_slk.data("Smin_G", tileset + "Sha"s));
	int blue = std::stoi(water_slk.data("Smin_B", tileset + "Sha"s));
	int alpha = std::stoi(water_slk.data("Smin_A", tileset + "Sha"s));

	shallow_color_min = { red, green, blue, alpha };

	red = std::stoi(water_slk.data("Smax_R", tileset + "Sha"s));
	green = std::stoi(water_slk.data("Smax_G", tileset + "Sha"s));
	blue = std::stoi(water_slk.data("Smax_B", tileset + "Sha"s));
	alpha = std::stoi(water_slk.data("Smax_A", tileset + "Sha"s));
	
	shallow_color_max = { red, green, blue, alpha };

	red = std::stoi(water_slk.data("Dmin_R", tileset + "Sha"s));
	green = std::stoi(water_slk.data("Dmin_G", tileset + "Sha"s));
	blue = std::stoi(water_slk.data("Dmin_B", tileset + "Sha"s));
	alpha = std::stoi(water_slk.data("Dmin_A", tileset + "Sha"s));

	deep_color_min = { red, green, blue, alpha };

	red = std::stoi(water_slk.data("Dmax_R", tileset + "Sha"s));
	green = std::stoi(water_slk.data("Dmax_G", tileset + "Sha"s));
	blue = std::stoi(water_slk.data("Dmax_B", tileset + "Sha"s));
	alpha = std::stoi(water_slk.data("Dmax_A", tileset + "Sha"s));

	deep_color_max = { red, green, blue, alpha };

	// Cliff Meshes
	slk::SLK cliffs("Data/Warcraft Data/Cliffs.slk", true);
	for (size_t i = 1; i < cliffs.rows; i++) {
		for (size_t j = 0; j < std::stoi(cliffs.data("variations", i)) + 1; j++) {
			std::string file_name = "Doodads\\Terrain\\Cliffs\\Cliffs" + cliffs.data("cliffID", i) + std::to_string(j) + ".mdx";
			cliff_meshes.push_back(resource_manager.load<StaticMesh>(file_name));
			path_to_cliff.emplace(cliffs.data("cliffID", i) + std::to_string(j), (int)cliff_meshes.size() - 1);
		}
		cliff_variations.emplace(cliffs.data("cliffID", i), std::stoi(cliffs.data("variations", i)));
	}

	ground_shader = resource_manager.load<Shader>({ "Data/Shaders/terrain.vs", "Data/Shaders/terrain.fs" });
	cliff_shader = resource_manager.load<Shader>({ "Data/Shaders/staticmesh.vs", "Data/Shaders/staticmesh.fs" });
	water_shader = resource_manager.load<Shader>({ "Data/Shaders/water.vs", "Data/Shaders/water.fs" });

	create();

	return true;
}

void Terrain::render() {
	// Render tiles
	ground_shader->use();

	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 MVP = camera.projection * camera.view * Model;

	gl->glUniformMatrix4fv(gl->glGetUniformLocation(ground_shader->program, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	gl->glActiveTexture(GL_TEXTURE0);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, ground_texture_array);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	gl->glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT, NULL);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);

	cliff_shader->use();
	// Render cliffs
	for (auto&& i : cliffs) {
		int min = std::min({  corner_cliff_height(corners[i.y * width + i.x]) ,			corner_cliff_height(corners[i.y * width + i.x + 1]), 
							  corner_cliff_height(corners[(i.y + 1) * width + i.x]),	corner_cliff_height(corners[(i.y + 1) * width + i.x + 1]) });

		Model = glm::translate(glm::mat4(1.0f), glm::vec3(i.x + 1, i.y, min));
		Model = glm::scale(Model, glm::vec3(1 / 128.f, 1 / 128.f, 1 / 128.f));
		MVP = camera.projection * camera.view * Model;
		gl->glUniformMatrix4fv(gl->glGetUniformLocation(cliff_shader->program, "MVP"), 1, GL_FALSE, &MVP[0][0]);

		cliff_meshes[i.z]->texture = cliff_texture_list[std::clamp(corners[i.y * width + i.x].cliff_texture, 0, 1)];
		cliff_meshes[i.z]->render();
	}

	// Render water
	water_shader->use();

	Model = glm::mat4(1.0f);
	MVP = camera.projection * camera.view * Model;

	gl->glUniformMatrix4fv(gl->glGetUniformLocation(water_shader->program, "MVP"), 1, GL_FALSE, &MVP[0][0]);
	gl->glUniform1i(gl->glGetUniformLocation(water_shader->program, "current_texture"), (int)current_texture);

	gl->glActiveTexture(GL_TEXTURE0);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, water_texture_array);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(2);
	gl->glBindBuffer(GL_ARRAY_BUFFER, water_color_buffer);
	gl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, water_index_buffer);
	gl->glDrawElements(GL_TRIANGLES, water_indices.size() * 3, GL_UNSIGNED_INT, NULL);

	gl->glDisableVertexAttribArray(2);
	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
}