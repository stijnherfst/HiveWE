#include "stdafx.h"

Terrain::~Terrain() {
	gl->glDeleteTextures(1, &ground_height);
	gl->glDeleteTextures(1, &ground_corner_height);
	gl->glDeleteTextures(1, &ground_texture_data);
	gl->glDeleteTextures(1, &cliff_texture_array);

	gl->glDeleteTextures(1, &water_texture_array);
	gl->glDeleteTextures(1, &water_exists);
	gl->glDeleteTextures(1, &water_height);
}

void Terrain::create() {
	// Ground textures
	for (auto&& tile_id : tileset_ids) {
		ground_textures.push_back(resource_manager.load<GroundTexture>(terrain_slk.data("dir", tile_id) + "/" + terrain_slk.data("file", tile_id) + ".blp"));
		ground_texture_to_id.emplace(tile_id, ground_textures.size() - 1);
	}
	blight_texture = ground_textures.size();
	ground_texture_to_id.emplace("blight", blight_texture);
	ground_textures.push_back(resource_manager.load<GroundTexture>("TerrainArt/Blight/Ashen_Blight.blp"));

	// Cliff Textures
	for (auto&& cliff_id : cliffset_ids) {
		auto t = cliff_slk.data("texDir", cliff_id) + "/" + cliff_slk.data("texFile", cliff_id) + ".blp";
		cliff_textures.push_back(resource_manager.load<Texture>(cliff_slk.data("texDir", cliff_id) + "/" + cliff_slk.data("texFile", cliff_id) + ".blp"));
		cliff_texture_size = std::max(cliff_texture_size, cliff_textures.back()->width);
		cliff_to_ground_texture.push_back(ground_texture_to_id[cliff_slk.data("groundTile", cliff_id)]);
	}

	ground_corner_heights.resize(width * height);
	ground_texture_list.resize((width - 1) * (height - 1));
	water_exists_data.resize(width * height);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			ground_corner_heights[j * width + i] = corner_height(i, j);

			water_exists_data[j * width + i] = corners[i][j].water;

			if (i == width - 1 || j == height - 1) {
				continue;
			}

			ground_texture_list[j * (width - 1) + i] = get_texture_variations(i, j);

			Corner& bottom_left = corners[i][j];
			Corner& bottom_right = corners[i + 1][j];
			Corner& top_left = corners[i][j + 1];
			Corner& top_right = corners[i + 1][j + 1];


			if (bottom_left.ramp) {
				//ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;
			}


			//if (bottom_left.cliff) {
			//	const int base = std::min({ bottom_left.layer_height, bottom_right.layer_height, top_left.layer_height, top_right.layer_height });

			//	if (bottom_left.ramp) {
			//		ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;

			//	}

			//	if (bottom_left.ramp != bottom_right.ramp) {
			////		ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;
			//	}
			//	if (bottom_left.ramp != top_left.ramp) {
			////		ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;
			//	}
			//}


			if (bottom_left.cliff) {
				const int base = std::min({ bottom_left.layer_height, bottom_right.layer_height, top_left.layer_height, top_right.layer_height });

				//if (bottom_left.ramp != bottom_right.ramp) {
				//	ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;

				//	// vertical
				//	char left_char = bottom_left.ramp ? 'L' : 'A';
				//	char right_char = bottom_right.ramp ? 'L' : 'A';

				//	std::string file_name = ""s
				//		+ char(left_char + (bottom_left.layer_height - base) * (bottom_left.ramp ? -4 : 1))
				//		+ char(left_char + (top_left.layer_height - base) * (bottom_left.ramp ? -4 : 1))
				//		+ char(right_char + (top_right.layer_height - base) * (bottom_right.ramp ? -4 : 1))
				//		+ char(right_char + (bottom_right.layer_height - base) * (bottom_right.ramp ? -4 : 1));

				//	file_name = "doodads/terrain/clifftrans/clifftrans" + file_name + "0.mdx";
				//	if (hierarchy.file_exists(file_name)) {

				//		if (path_to_cliff.find(file_name) == path_to_cliff.end()) {


				//			cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name));
				//			path_to_cliff.emplace(file_name, static_cast<int>(cliff_meshes.size()) - 1);
				//		}

				//		cliffs.emplace_back(i, j - (top_left.layer_height > bottom_left.layer_height), path_to_cliff[file_name]);
				//	}
				//	continue;
				//}
				//if (bottom_left.ramp != top_left.ramp) {
				//	ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;

				//	// horizontal
				//	char bottom_char = bottom_left.ramp ? 'L' : 'A';
				//	char top_char = top_left.ramp ? 'L' : 'A';

				//	std::string file_name = ""s
				//		+ char(bottom_char + (bottom_left.layer_height - base) * (bottom_left.ramp ? -4 : 1))
				//		+ char(top_char + (top_left.layer_height - base) * (top_left.ramp ? -4 : 1))
				//		+ char(top_char + (top_right.layer_height - base) * (top_left.ramp ? -4 : 1))
				//		+ char(bottom_char + (bottom_right.layer_height - base) * (bottom_left.ramp ? -4 : 1));

				//	file_name = "doodads/terrain/clifftrans/clifftrans" + file_name + "0.mdx";
				//	if (hierarchy.file_exists(file_name)) {
				//		if (path_to_cliff.find(file_name) == path_to_cliff.end()) {


				//			cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name));
				//			path_to_cliff.emplace(file_name, static_cast<int>(cliff_meshes.size()) - 1);
				//		}

				//		cliffs.emplace_back(i + (bottom_left.layer_height > bottom_right.layer_height), j, path_to_cliff[file_name]);
				//	}
				//	continue;
				//}

				//if (bottom_left.ramp) {
				//	continue;
				//}
				ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;

				// Cliff model path
				std::string file_name = ""s + char('A' + bottom_left.layer_height - base)
					+ char('A' + top_left.layer_height - base)
					+ char('A' + top_right.layer_height - base)
					+ char('A' + bottom_right.layer_height - base);

				if (file_name == "AAAA") {
					continue;
				}

				// Clamp to within max variations
				file_name += std::to_string(std::clamp(bottom_left.cliff_variation, 0, cliff_variations[file_name]));

				cliffs.emplace_back(i, j, path_to_cliff[file_name]);
			}
		}
	}

	// Ground
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_texture_data);
	gl->glTextureStorage2D(ground_texture_data, 1, GL_RGBA16UI, width - 1, height - 1);
	gl->glTextureSubImage2D(ground_texture_data, 0, 0, 0, width - 1, height - 1, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, ground_texture_list.data());
	gl->glTextureParameteri(ground_texture_data, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(ground_texture_data, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_height);
	gl->glTextureStorage2D(ground_height, 1, GL_R16F, width, height);
	gl->glTextureSubImage2D(ground_height, 0, 0, 0, width, height, GL_RED, GL_FLOAT, ground_heights.data());
	gl->glTextureParameteri(ground_height, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(ground_height, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &ground_corner_height);
	gl->glTextureStorage2D(ground_corner_height, 1, GL_R16F, width, height);
	gl->glTextureSubImage2D(ground_corner_height, 0, 0, 0, width, height, GL_RED, GL_FLOAT, ground_corner_heights.data());

	// Cliff
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &cliff_texture_array);
	gl->glTextureStorage3D(cliff_texture_array, log2(cliff_texture_size) + 1, GL_RGBA8, cliff_texture_size, cliff_texture_size, cliff_textures.size());

	int sub = 0;
	for (auto&& i : cliff_textures) {
		gl->glTextureSubImage3D(cliff_texture_array, 0, 0, 0, sub, i->width, i->height, 1, GL_BGRA, GL_UNSIGNED_BYTE, i->data.data());
		sub += 1;
	}
	gl->glGenerateTextureMipmap(cliff_texture_array);

	// Water
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &water_height);
	gl->glTextureStorage2D(water_height, 1, GL_R16F, width, height);
	gl->glTextureSubImage2D(water_height, 0, 0, 0, width, height, GL_RED, GL_FLOAT, water_heights.data());

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &water_exists);
	gl->glTextureStorage2D(water_exists, 1, GL_R8, width, height);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gl->glTextureSubImage2D(water_exists, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, water_exists_data.data());
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// Water textures
	gl->glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &water_texture_array);
	gl->glTextureStorage3D(water_texture_array, std::log(128) + 1, GL_RGBA8, 128, 128, water_textures_nr);
	gl->glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (size_t i = 0; i < water_textures.size(); i++) {
		if (water_textures[i]->width != 128 || water_textures[i]->height != 128) {
			std::cout << "Odd water texture size detected of " << water_textures[i]->width << " wide and " << water_textures[i]->height << " high\n";
		}
		gl->glTextureSubImage3D(water_texture_array, 0, 0, 0, i, water_textures[i]->width, water_textures[i]->height, 1, GL_BGRA, GL_UNSIGNED_BYTE, water_textures[i]->data.data());
	}
	gl->glGenerateTextureMipmap(water_texture_array);
}

bool Terrain::load(BinaryReader& reader) {
	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3E!") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
		return false;
	}
	// Version
	reader.read<uint32_t>();

	tileset = reader.read<char>();
	reader.advance(4); // Custom tileset

	const uint32_t tileset_textures = reader.read<uint32_t>();
	for (size_t i = 0; i < tileset_textures; i++) {
		tileset_ids.push_back(reader.read_string(4));
	}

	const int cliffset_textures = reader.read<uint32_t>();
	for (int i = 0; i < cliffset_textures; i++) {
		cliffset_ids.push_back(reader.read_string(4));
	}

	width = reader.read<uint32_t>();
	height = reader.read<uint32_t>();

	offset = reader.read<glm::vec2>();

	// Parse all tilepoints
	corners.resize(width, std::vector<Corner>(height));
	ground_heights.resize(width * height);
	water_heights.resize(width * height);
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			Corner& corner = corners[i][j];

			ground_heights[j * width + i] = (reader.read<uint16_t>() - 8192.f) / 512.f;

			const uint16_t water_and_edge = reader.read<uint16_t>();
			water_heights[j * width + i] = ((water_and_edge & 0x3FFF) - 8192.f) / 512.f;
			corner.map_edge = water_and_edge & 0x4000;

			const uint8_t texture_and_flags = reader.read<uint8_t>();
			corner.ground_texture = texture_and_flags & 0b00001111;

			corner.ramp = texture_and_flags & 0b00010000;
			corner.blight = texture_and_flags & 0b00100000;
			corner.water = texture_and_flags & 0b01000000;
			corner.boundary = texture_and_flags & 0b10000000;

			const uint8_t variation = reader.read<uint8_t>();
			corner.ground_variation = variation & 0b00011111;
			corner.cliff_variation = (variation & 0b11100000) >> 5;

			const uint8_t misc = reader.read<uint8_t>();
			corner.cliff_texture = (misc & 0b11110000) >> 4;
			corner.layer_height = misc & 0b00001111;
		}
	}

	// Determine if cliff
	for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			Corner& bottom_left = corners[i][j];
			Corner& bottom_right = corners[i + 1][j];
			Corner& top_left = corners[i][j + 1];
			Corner& top_right = corners[i + 1][j + 1];

			bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height
				|| bottom_left.layer_height != top_left.layer_height
				|| bottom_left.layer_height != top_right.layer_height;
		}
	}
	// Done parsing

	hierarchy.tileset = tileset;

	terrain_slk.load("TerrainArt/Terrain.slk");
	cliff_slk.load("TerrainArt/CliffTypes.slk");
	slk::SLK water_slk("TerrainArt/Water.slk");

	// Water Textures and Colours

	water_offset = water_slk.data<float>("height", tileset + "Sha"s);
	water_textures_nr = water_slk.data<int>("numTex", tileset + "Sha"s);
	animation_rate = water_slk.data<int>("texRate", tileset + "Sha"s);

	std::string file_name = water_slk.data("texFile", tileset + "Sha"s);
	for (int i = 0; i < water_textures_nr; i++) {
		water_textures.push_back(resource_manager.load<Texture>(file_name + (i < 10 ? "0" : "") + std::to_string(i) + ".blp"));
	}

	int red =	water_slk.data<int>("Smin_R", tileset + "Sha"s);
	int green = water_slk.data<int>("Smin_G", tileset + "Sha"s);
	int blue =	water_slk.data<int>("Smin_B", tileset + "Sha"s);
	int alpha = water_slk.data<int>("Smin_A", tileset + "Sha"s);

	shallow_color_min = { red, green, blue, alpha };
	shallow_color_min /= 255.f;

	red =	water_slk.data<int>("Smax_R", tileset + "Sha"s);
	green = water_slk.data<int>("Smax_G", tileset + "Sha"s);
	blue =	water_slk.data<int>("Smax_B", tileset + "Sha"s);
	alpha = water_slk.data<int>("Smax_A", tileset + "Sha"s);

	shallow_color_max = { red, green, blue, alpha };
	shallow_color_max /= 255.f;

	red =	water_slk.data<int>("Dmin_R", tileset + "Sha"s);
	green = water_slk.data<int>("Dmin_G", tileset + "Sha"s);
	blue =	water_slk.data<int>("Dmin_B", tileset + "Sha"s);
	alpha = water_slk.data<int>("Dmin_A", tileset + "Sha"s);

	deep_color_min = { red, green, blue, alpha };
	deep_color_min /= 255.f;

	red =	water_slk.data<int>("Dmax_R", tileset + "Sha"s);
	green = water_slk.data<int>("Dmax_G", tileset + "Sha"s);
	blue =	water_slk.data<int>("Dmax_B", tileset + "Sha"s);
	alpha = water_slk.data<int>("Dmax_A", tileset + "Sha"s);

	deep_color_max = { red, green, blue, alpha };
	deep_color_max /= 255.f;

	// Cliff Meshes
	slk::SLK cliffs_slk("Data/Warcraft Data/Cliffs.slk", true);
	for (size_t i = 1; i < cliffs_slk.rows; i++) {
		for (int j = 0; j < cliffs_slk.data<int>("variations", i) + 1; j++) {
			file_name = "Doodads/Terrain/Cliffs/Cliffs" + cliffs_slk.data("cliffID", i) + std::to_string(j) + ".mdx";
			cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name));
			path_to_cliff.emplace(cliffs_slk.data("cliffID", i) + std::to_string(j), static_cast<int>(cliff_meshes.size()) - 1);
		}
		cliff_variations.emplace(cliffs_slk.data("cliffID", i), cliffs_slk.data<int>("variations", i));
	}

	ground_shader = resource_manager.load<Shader>({ "Data/Shaders/terrain.vs", "Data/Shaders/terrain.fs" });
	cliff_shader = resource_manager.load<Shader>({ "Data/Shaders/cliff.vs", "Data/Shaders/cliff.fs" });
	water_shader = resource_manager.load<Shader>({ "Data/Shaders/water.vs", "Data/Shaders/water.fs" });

	create();

	return true;
}

void Terrain::save() const {
	BinaryWriter writer;
	writer.write_string("W3E!");
	writer.write(write_version);
	writer.write(tileset);
	writer.write(1);
	writer.write<uint32_t>(tileset_ids.size());
	writer.write_vector(tileset_ids);
	writer.write<uint32_t>(cliffset_ids.size());
	writer.write_vector(cliffset_ids);
	writer.write(width);
	writer.write(height);
	writer.write(offset);

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			const Corner& corner = corners[i][j];

			writer.write<uint16_t>(ground_heights[j * width + i] * 512.f + 8192.f);

			uint16_t water_and_edge = water_heights[j * width + i] * 512.f + 8192.f;
			water_and_edge += corner.map_edge << 14;
			writer.write(water_and_edge);

			uint8_t texture_and_flags = corner.ground_texture;
			texture_and_flags |= corner.ramp << 4;

			texture_and_flags |= corner.blight << 5;
			texture_and_flags |= corner.water << 6;
			texture_and_flags |= corner.boundary << 7;
			writer.write(texture_and_flags);

			uint8_t variation = corner.ground_variation;
			variation += corner.cliff_variation << 5;
			writer.write(variation);

			uint8_t misc = corner.cliff_texture << 4;
			misc += corner.layer_height;
			writer.write(misc);
		}
	}

	HANDLE handle;
	const bool success = SFileCreateFile(hierarchy.map.handle, "war3map.w3e", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	SFileFinishFile(handle);
}

void Terrain::render() const {
	// Render tiles
	auto begin = std::chrono::high_resolution_clock::now();

	ground_shader->use();

	gl->glDisable(GL_BLEND);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera->projection_view[0][0]);
	gl->glUniform1i(2, map->render_pathing);
	gl->glUniform1i(3, map->render_lighting);

	gl->glBindTextureUnit(0, ground_height);
	gl->glBindTextureUnit(1, ground_corner_height);
	gl->glBindTextureUnit(2, ground_texture_data);

	for (size_t i = 0; i < ground_textures.size(); i++) {
		gl->glBindTextureUnit(3 + i, ground_textures[i]->id);
	}
	gl->glBindTextureUnit(20, map->pathing_map.texture_static);
	gl->glBindTextureUnit(21, map->pathing_map.texture_dynamic);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, (width - 1) * (height - 1));

	gl->glDisableVertexAttribArray(0);

	gl->glEnable(GL_BLEND);

	auto end = std::chrono::high_resolution_clock::now();
	map->terrain_tiles_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render cliffs
	begin = std::chrono::high_resolution_clock::now();

	for (auto&& i : cliffs) {
		const Corner& bottom_left = corners[i.x][i.y];
		const Corner& bottom_right = corners[i.x + 1][i.y];
		const Corner& top_left = corners[i.x][i.y + 1];
		const Corner& top_right = corners[i.x + 1][i.y + 1];

		const float min = std::min({bottom_left.layer_height - 2,	bottom_right.layer_height - 2,
									top_left.layer_height - 2,		top_right.layer_height - 2 });

		cliff_meshes[i.z]->render_queue({ i.x, i.y, min, bottom_left.cliff_texture });
	}

	cliff_shader->use();

	// WC3 models are 128x too large
	glm::mat4 MVP = glm::scale(camera->projection_view, glm::vec3(1.f / 128.f));
	gl->glUniformMatrix4fv(0, 1, GL_FALSE, &MVP[0][0]);
	gl->glUniform1i(1, map->render_pathing);
	gl->glUniform1i(2, map->render_lighting);

	gl->glBindTextureUnit(0, cliff_texture_array);
	gl->glBindTextureUnit(1, ground_height);
	gl->glBindTextureUnit(2, map->pathing_map.texture_static);
	for (auto&& i : cliff_meshes) {
		i->render();
	}

	end = std::chrono::high_resolution_clock::now();
	map->terrain_cliff_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render water
	begin = std::chrono::high_resolution_clock::now();

	water_shader->use();

	gl->glUniformMatrix4fv(0, 1, GL_FALSE, &camera->projection_view[0][0]);
	gl->glUniform4fv(1, 1, &shallow_color_min[0]);
	gl->glUniform4fv(2, 1, &shallow_color_max[0]);
	gl->glUniform4fv(3, 1, &deep_color_min[0]);
	gl->glUniform4fv(4, 1, &deep_color_max[0]);
	gl->glUniform1f(5, water_offset);
	gl->glUniform1i(6, current_texture);

	gl->glBindTextureUnit(0, water_height);
	gl->glBindTextureUnit(1, ground_corner_height);
	gl->glBindTextureUnit(2, water_exists);
	gl->glBindTextureUnit(3, water_texture_array);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, (width - 1) * (height - 1));

	gl->glDisableVertexAttribArray(0);

	gl->glEnable(GL_BLEND);

	end = std::chrono::high_resolution_clock::now();
	map->terrain_water_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;
}

void Terrain::change_tileset(const std::vector<std::string>& new_tileset_ids, std::vector<int> new_to_old) {
	tileset_ids = new_tileset_ids;

	// Blight
	new_to_old.push_back(new_tileset_ids.size());

	// Map old ids to the new ids
	for (auto&& i : corners) {
		for (auto&& j : i) {
			j.ground_texture = new_to_old[j.ground_texture];
		}
	}

	// Reload tile textures
	ground_textures.clear();	// ToDo Clear them after loading new ones?
	ground_texture_to_id.clear();

	for (auto&& tile_id : tileset_ids) {
		ground_textures.push_back(resource_manager.load<GroundTexture>(terrain_slk.data("dir", tile_id) + "/" + terrain_slk.data("file", tile_id) + ".blp"));
		ground_texture_to_id.emplace(tile_id, ground_textures.size() - 1);
	}
	blight_texture = ground_textures.size();
	ground_texture_to_id.emplace("blight", blight_texture);
	ground_textures.push_back(resource_manager.load<GroundTexture>("TerrainArt/Blight/Ashen_Blight.blp"));

	cliff_to_ground_texture.clear();
	for (auto&& cliff_id : cliffset_ids) {
		cliff_to_ground_texture.push_back(ground_texture_to_id[cliff_slk.data("groundTile", cliff_id)]);
	}

	// Update texture usage information
	for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			ground_texture_list[j * (width - 1) + i] = get_texture_variations(i, j);

			if (corners[i][j].cliff) {
				ground_texture_list[j * (width - 1) + i].a |= 0b1000000000000000;
			}
		}
	}

	gl->glTextureSubImage2D(ground_texture_data, 0, 0, 0, width - 1, height - 1, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, ground_texture_list.data());
}

/// The final height a tilepoint will have in the terrain
float Terrain::corner_height(const int x, const int y) const {
	return ground_heights[y * width + x] + corners[x][y].layer_height - 2.0;
}

/// The final height the water point will have in the terrain
float Terrain::corner_water_height(const int x, const int y) const {
	return water_heights[y * width + x] + water_offset;
}

/// The texture of the tilepoint which is influenced by its surroundings. nearby cliff > blight > regular texture
int Terrain::real_tile_texture(const int x, const int y) const {
	for (int i = -1; i < 1; i++) {
		for (int j = -1; j < 1; j++) {
			if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) {
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

/// The subtexture of a groundtexture to use.
int Terrain::get_tile_variation(const int ground_texture, const int variation) const {
	if (ground_textures[ground_texture]->extended) {
		if (variation <= 15) {
			return 16 + variation;
		} else if (variation == 16) {
			return 15;
		} else {
			return 0;
		}
	} else {
		if (variation == 0) {
			return 0;
		} else {
			return 15;
		}
	}
}

/// The 4 ground textures of the tilepoint. First 5 bits are which texture array to use and the next 5 bits are which subtexture to use
glm::u16vec4 Terrain::get_texture_variations(const int x, const int y) const {
	const int bottom_left = real_tile_texture(x, y);
	const int bottom_right = real_tile_texture(x + 1, y);
	const int top_left = real_tile_texture(x, y + 1);
	const int top_right = real_tile_texture(x + 1, y + 1);

	std::set<int> set({ bottom_left, bottom_right, top_left, top_right });
	glm::u16vec4 tiles(17); // 17 means black transparent texture
	int component = 1;

	tiles.x = *set.begin() + (get_tile_variation(*set.begin(), corners[x][y].ground_variation) << 5);
	set.erase(set.begin());

	std::bitset<4> index;
	for (auto&& texture : set) {
		index[0] = bottom_right == texture;
		index[1] = bottom_left == texture;
		index[2] = top_right == texture;
		index[3] = top_left	== texture;

		tiles[component++] = texture + (index.to_ulong() << 5);
	}
	return tiles;
}

/// Constructs a minimap image with tile, cliff, and water colors. Other objects such ad doodads will not be added here
Texture Terrain::minimap_image() {
	Texture new_minimap_image;

	new_minimap_image.width = width;
	new_minimap_image.height = height;
	new_minimap_image.channels = 4;
	new_minimap_image.data.resize(width * height * 4);
	
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			glm::vec4 color;

			if (corners[i][j].water && corner_water_height(i, j) > corner_height(i, j)) {
				color = glm::vec4(50, 50, 200, 255);
			} else if (corners[i][j].cliff || (i > 0 && corners[i - 1][j].cliff) || (j > 0 && corners[i][j - 1].cliff) || (i > 0 && j > 0 && corners[i - 1][j - 1].cliff)) {
				color = cliff_textures[std::min(1, corners[i][j].cliff_texture)]->clr;
			} else {
				color = ground_textures[real_tile_texture(i, j)]->minimap_color;
			}

			int index = (height - 1 - j) * (width * 4) + i * 4;
			new_minimap_image.data[index + 0] = color.r;
			new_minimap_image.data[index + 1] = color.g;
			new_minimap_image.data[index + 2] = color.b;
			new_minimap_image.data[index + 3] = color.a;

		}
	}

	return new_minimap_image;
}