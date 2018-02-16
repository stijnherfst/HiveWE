#include "stdafx.h"

Terrain::~Terrain() {
	gl->glDeleteTextures(1, &ground_height);
	gl->glDeleteTextures(1, &ground_corner_height);
	gl->glDeleteTextures(1, &ground_texture_data);

	gl->glDeleteBuffers(1, &water_vertex_buffer);
	gl->glDeleteBuffers(1, &water_uv_buffer);
	gl->glDeleteBuffers(1, &water_color_buffer);
	gl->glDeleteBuffers(1, &water_index_buffer);

	gl->glDeleteTextures(1, &ground_texture_array);
	gl->glDeleteTextures(1, &cliff_texture_array);
	gl->glDeleteTextures(1, &water_texture_array);
}

void Terrain::create() {
	ground_heights.resize(width * height);
	ground_corner_heights.resize(width * height);
	ground_texture_list.resize(width * height);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			Corner& bottomLeft = corners[i][j];
			Corner& bottomRight = corners[i + 1][j];
			Corner& topLeft = corners[i][j + 1];
			Corner& topRight = corners[i + 1][j + 1];

			// Ground tiles
			ground_heights[j * width + i] = bottomLeft.ground_height;
			ground_corner_heights[j * width + i] = corner_height(bottomLeft);
			ground_texture_list[j * width + i] = get_texture_variations(i, j);

			if (bottomLeft.cliff) {
				ground_texture_list[j * width + i].a |= 0b1000000000000000;

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
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_texture_data);
	gl->glTextureStorage2D(ground_texture_data, 1, GL_RGBA16UI, width, height);
	gl->glTextureSubImage2D(ground_texture_data, 0, 0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, ground_texture_list.data());
	gl->glTextureParameteri(ground_texture_data, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(ground_texture_data, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_height);
	gl->glTextureStorage2D(ground_height, 1, GL_R16F, width, height);
	gl->glTextureSubImage2D(ground_height, 0, 0, 0, width, height, GL_RED, GL_FLOAT, ground_heights.data());

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_corner_height);
	gl->glTextureStorage2D(ground_corner_height, 1, GL_R16F, width, height);
	gl->glTextureSubImage2D(ground_corner_height, 0, 0, 0, width, height, GL_RED, GL_FLOAT, ground_corner_heights.data());

	// Ground textures
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &ground_texture_array);
	gl->glTextureStorage3D(ground_texture_array, std::log(variation_size) + 1, GL_RGBA8, variation_size, variation_size, ground_textures.size() * 32 + 1); // Index 0 is a transparant black texture
	gl->glTextureParameteri(ground_texture_array, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(ground_texture_array, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create a transparant black texture
	//gl->glClearTexSubImage(ground_texture_array, 0, 0, 0, 0, variation_size, variation_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

	int sub = 0;
	for (auto&& i : ground_textures) {
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, i->width);
		for (size_t y = 0; y < 4; y++) {
			for (size_t x = 0; x < 4; x++) {
				int sub_image = sub * 32 + y * 4 + x;
				gl->glTextureSubImage3D(ground_texture_array, 0, 0, 0, sub_image + 1, variation_size, variation_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, i->data + (y * variation_size * i->width + x * variation_size) * 4);

				// If extended
				if (i->width == i->height * 2) {
					gl->glTextureSubImage3D(ground_texture_array, 0, 0, 0, sub_image + 1 + 16, variation_size, variation_size, 1, GL_BGRA, GL_UNSIGNED_BYTE, i->data + (y * variation_size * i->width + (x + 4) * variation_size) * 4);
				}
			}
		}
		sub += 1;
	}
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	gl->glGenerateTextureMipmap(ground_texture_array);

	// Cliff
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &cliff_texture_array);
	gl->glTextureStorage3D(cliff_texture_array, std::log(cliff_texture_size) + 1, GL_RGBA8, cliff_texture_size, cliff_texture_size, cliff_textures.size());

	sub = 0;
	for (auto&& i : cliff_textures) {
		gl->glTextureSubImage3D(cliff_texture_array, 0, 0, 0, sub, i->width, i->height, 1, GL_BGRA, GL_UNSIGNED_BYTE, i->data);
		sub += 1;
	}
	gl->glGenerateTextureMipmap(cliff_texture_array);

	// Water
	gl->glCreateBuffers(1, &water_vertex_buffer);
	gl->glNamedBufferData(water_vertex_buffer, water_vertices.size() * sizeof(glm::vec3), water_vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &water_uv_buffer);
	gl->glNamedBufferData(water_uv_buffer, water_uvs.size() * sizeof(glm::vec2), water_uvs.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &water_color_buffer);
	gl->glNamedBufferData(water_color_buffer, water_colors.size() * sizeof(glm::vec4), water_colors.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &water_index_buffer);
	gl->glNamedBufferData(water_index_buffer, water_indices.size() * sizeof(unsigned int) * 3, water_indices.data(), GL_STATIC_DRAW);

	// Water textures
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &water_texture_array);
	gl->glTextureStorage3D(water_texture_array, std::log(128) + 1, GL_RGBA8, 128, 128, water_textures_nr);
	gl->glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (size_t i = 0; i < water_textures.size(); i++) {
		if (water_textures[i]->width > 128 || water_textures[i]->height > 128) {
			std::cout << "Odd water texture size detected of " << water_textures[i]->width << " wide and " << water_textures[i]->height << " high\n";
			continue;
		}
		gl->glTextureSubImage3D(water_texture_array, 0, 0, 0, i, 128, 128, 1, GL_BGRA, GL_UNSIGNED_BYTE, water_textures[i]->data);
	}
	gl->glGenerateTextureMipmap(water_texture_array);
}

bool Terrain::load(BinaryReader& reader) {
	std::string magic_number = reader.read_string(4);
	if (magic_number != "W3E!") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
		return false;
	}
	// Version
	reader.read<uint32_t>();

	tileset = reader.read<char>();
	bool custom_tileset = reader.read<uint32_t>() == 1 ? true : false; // 0 for not default, 1 for custom

	uint32_t tileset_textures = reader.read<uint32_t>();
	for (size_t i = 0; i < tileset_textures; i++) {
		tileset_ids.push_back(reader.read_string(4));
	}

	int cliffset_textures = reader.read<uint32_t>();
	for (size_t i = 0; i < cliffset_textures; i++) {
		cliffset_ids.push_back(reader.read_string(4));
	}

	width = reader.read<uint32_t>() - 1;
	height = reader.read<uint32_t>() - 1;

	offset = reader.read<glm::vec2>();

	// Parse all tilepoints
	corners.resize(width + 1, std::vector<Corner>(height + 1));
	for (size_t j = 0; j < height + 1; j++) {
		for (size_t i = 0; i < width + 1; i++) {
			Corner& corner = corners[i][j];

			corner.ground_height = (reader.read<uint16_t>() - 8192.f) / 512.f;

			uint16_t water_and_edge = reader.read<uint16_t>();
			corner.water_height = ((water_and_edge & 0x3FFF) - 8192.f)/ 512.f;
			corner.map_edge = water_and_edge & 0x4000;

			uint8_t texture_and_flags = reader.read<uint8_t>();
			corner.ground_texture = texture_and_flags & 0b00001111;
			 
			corner.ramp = texture_and_flags & 0b00010000;
			corner.blight = texture_and_flags & 0b00100000;
			corner.water = texture_and_flags & 0b01000000;
			corner.boundary = texture_and_flags & 0b10000000;

			uint8_t variation = reader.read<uint8_t>();
			corner.ground_variation = variation & 0b00011111;
			corner.cliff_variation = (variation & 0b11100000) >> 5;

			uint8_t misc = reader.read<uint8_t>();
			corner.cliff_texture = (misc & 0b11110000) >> 4;
			corner.layer_height = misc & 0b00001111;
		}
	}

	// Determine if cliff
	for (size_t i = 0; i < width; i++) {
		for (size_t j = 0; j < height; j++) {
			Corner& bottomLeft = corners[i][j];
			Corner& bottomRight = corners[i + 1][j];
			Corner& topLeft = corners[i][j + 1];
			Corner& topRight = corners[i + 1][j + 1];

			if (bottomLeft.layer_height != bottomRight.layer_height
				|| bottomLeft.layer_height != topLeft.layer_height
				|| bottomLeft.layer_height != topRight.layer_height) {

				bottomLeft.cliff = true;
			}
		}
	}
	// Done parsing

	hierarchy.load_tileset(tileset);

	// Ground Textures
	terrain_slk = slk::SLK("TerrainArt\\Terrain.slk");
	for (auto&& tile_id : tileset_ids) {
		ground_textures.push_back(resource_manager.load<Texture>(terrain_slk.data("dir", tile_id) + "\\" + terrain_slk.data("file", tile_id) + ".blp"));
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
	slk::SLK cliffs_slk("Data/Warcraft Data/Cliffs.slk", true);
	for (size_t i = 1; i < cliffs_slk.rows; i++) {
		for (size_t j = 0; j < std::stoi(cliffs_slk.data("variations", i)) + 1; j++) {
			file_name = "Doodads\\Terrain\\Cliffs\\Cliffs" + cliffs_slk.data("cliffID", i) + std::to_string(j) + ".mdx";
			cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name));
			path_to_cliff.emplace(cliffs_slk.data("cliffID", i) + std::to_string(j), (int)cliff_meshes.size() - 1);
		}
		cliff_variations.emplace(cliffs_slk.data("cliffID", i), std::stoi(cliffs_slk.data("variations", i)));
	}

	ground_shader = resource_manager.load<Shader>({ "Data/Shaders/terrain.vs", "Data/Shaders/terrain.fs" });
	cliff_shader = resource_manager.load<Shader>({ "Data/Shaders/cliff.vs", "Data/Shaders/cliff.fs" });
	water_shader = resource_manager.load<Shader>({ "Data/Shaders/water.vs", "Data/Shaders/water.fs" });

	create();

	return true;
}

void Terrain::render() {
	// Render tiles

	auto begin = std::chrono::high_resolution_clock::now();

	ground_shader->use();

	gl->glDisable(GL_BLEND);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
	gl->glUniform1i(2, map.render_pathing);

	gl->glBindTextureUnit(0, ground_texture_array);
	gl->glBindTextureUnit(1, ground_corner_height);
	gl->glBindTextureUnit(2, ground_texture_data);
	gl->glBindTextureUnit(3, pathing_map_texture);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, width * height);

	gl->glDisableVertexAttribArray(0);

	gl->glEnable(GL_BLEND);

	auto end = std::chrono::high_resolution_clock::now();
	map.terrain_tiles_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render cliffs
	begin = std::chrono::high_resolution_clock::now();

	for (auto&& i : cliffs) {
		Corner& bottomLeft = corners[i.x][i.y];
		Corner& bottomRight = corners[i.x + 1][i.y];
		Corner& topLeft = corners[i.x][i.y + 1];
		Corner& topRight = corners[i.x + 1][i.y + 1];

		float min = std::min({	bottomLeft.layer_height - 2,bottomRight.layer_height - 2,
								topLeft.layer_height - 2,	topRight.layer_height - 2 });

		cliff_meshes[i.z]->render_queue({ i.x, i.y, min, bottomLeft.cliff_texture });
	}
	
	cliff_shader->use();
	glm::mat4 MVP = glm::scale(camera.projection_view, glm::vec3(1.f / 128.f));
	gl->glUniformMatrix4fv(3, 1, GL_FALSE, &MVP[0][0]);

	gl->glBindTextureUnit(0, cliff_texture_array);
	gl->glBindTextureUnit(1, ground_height);
	for (auto&& i : cliff_meshes) {
		i->render();
	}

	end = std::chrono::high_resolution_clock::now();
	map.terrain_cliff_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render water
	begin = std::chrono::high_resolution_clock::now();

	water_shader->use();

	gl->glUniformMatrix4fv(3, 1, GL_FALSE, &camera.projection_view[0][0]);
	gl->glUniform1i(4, current_texture);

	gl->glBindTextureUnit(0, water_texture_array);

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

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glDisableVertexAttribArray(2);

	end = std::chrono::high_resolution_clock::now();
	map.terrain_water_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;
}

float Terrain::corner_height(Corner corner) const {
	return corner.ground_height + corner.layer_height - 2.0;
}

float Terrain::corner_height(const int x, const int y) const {
	return corners[x][y].ground_height + corners[x][y].layer_height - 2.0;
}

float Terrain::corner_water_height(Corner corner) const {
	return corner.water_height + height_offset;
}

int Terrain::real_tile_texture(int x, int y) {
	for (int i = -1; i < 1; i++) {
		for (int j = -1; j < 1; j++) {
			if (x + i >= 0 && x + i <= width && y + j >= 0 && y + j <= height) {
				if (corners[x + i][y + j].cliff) {
					int texture = corners[x + i][y + j].cliff_texture;
					// Number 15 seems to be something
					if (texture == 15) {
						texture -= 14;
					}
					return cliff_to_ground_texture[texture];
				}
			}
		}
	}
	if (corners[x][y].blight) {
		return blight_texture;
	}

	return corners[x][y].ground_texture;
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

glm::u16vec4 Terrain::get_texture_variations(int x, int y) {
	glm::u16vec4 tiles;
	// Bottom and top reversed
	auto bottomL = std::make_tuple(real_tile_texture(x, y + 1), corners[x][y + 1]);
	auto bottomR = std::make_tuple(real_tile_texture(x + 1, y + 1), corners[x + 1][y + 1]);
	auto topL = std::make_tuple(real_tile_texture(x, y), corners[x][y]);
	auto topR = std::make_tuple(real_tile_texture(x + 1, y), corners[x + 1][y]);

	auto comp = [&](std::tuple<int, Corner> l, std::tuple<int, Corner> r) {
		return std::get<0>(l) < std::get<0>(r);
	};

	std::set<std::tuple<int, Corner>, decltype(comp)> set({ topL, topR, bottomL, bottomR }, comp);

	auto[texture, corner] = *set.begin();
	tiles.x = texture * 32 + get_tile_variation(corner) + 1; // Texture 0 is black and fully transparant
	set.erase(set.begin());

	int component = 1;
	std::bitset<4> index;
	for (auto[texture, corner] : set) {
		// Bottom and top reversed
		index[0] = real_tile_texture(x + 1, y + 1) == texture;
		index[1] = real_tile_texture(x, y + 1) == texture;
		index[2] = real_tile_texture(x + 1, y) == texture;
		index[3] = real_tile_texture(x, y) == texture;

		switch (component) {
		case 1:
			tiles.y = texture * 32 + index.to_ulong() + 1;
			break;
		case 2:
			tiles.z = texture * 32 + index.to_ulong() + 1;
			break;
		case 3:
			tiles.w = texture * 32 + index.to_ulong() + 1;
			break;
		}
		component += 1;
	}
	return tiles;
}