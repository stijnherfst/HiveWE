#include "stdafx.h"

// This whole class needs a bit of a rework

TerrainBrush::TerrainBrush() {
	size_granularity = 4;
	uv_offset_locked = true;
	uv_offset = { 2, 2 };
	brush_offset = { 0.5f, 0.5f };
}

// Make this an iterative function instead to avoid stack overflows
void TerrainBrush::check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const {
	QRect bounds = QRect(i - 1, j - 1, 3, 3).intersected({ 0, 0, map->terrain.width, map->terrain.height });

	for (int k = bounds.x(); k <= bounds.right(); k++) {
		for (int l = bounds.y(); l <= bounds.bottom(); l++) {
			if (k == 0 && l == 0) {
				continue;
			}

			int difference = map->terrain.corners[i][j].layer_height - map->terrain.corners[k][l].layer_height;
			if (std::abs(difference) > 2 && !contains(begx + (k - i), begy + (l - k))) {
				map->terrain.corners[k][l].layer_height = map->terrain.corners[i][j].layer_height - std::clamp(difference, -2, 2);
				map->terrain.ground_corner_heights[l * map->terrain.width + k] = map->terrain.corner_height(k, l);

				area.setX(std::min(area.x(), k - 1));
				area.setY(std::min(area.y(), l - 1));
				area.setRight(std::max(area.right(), k));
				area.setBottom(std::max(area.bottom(), l));

				check_nearby(begx, begy, k, l, area);
			}
		}
	}
};

void TerrainBrush::apply() {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	auto& ground_heights = map->terrain.ground_heights;
	auto& water_heights = map->terrain.water_heights;

	const int x = position.x + 1;
	const int y = position.y + 1;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, width, height });
	QRect area2 = QRect(x - 1, y - 1, cells + 1, cells + 1).intersected({ 0, 0, width - 1, height - 1 });
	QRect updated_area = area2;

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	if (apply_texture) {
		const int id = map->terrain.ground_texture_to_id[tile_id];

		// Update textures
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				if (!contains(i - area.x() - std::min(position.x + 1, 0), j - area.y() - std::min(position.y + 1, 0))) {
					continue;
				}

				bool cliff_near = false;
				for (int k = -1; k < 1 && !cliff_near; k++) {
					for (int l = -1; l < 1 && !cliff_near; l++) {
						if (i + k >= 0 && i + k <= width && j + l >= 0 && j + l <= height) {
							cliff_near = corners[i + k][j + l].cliff;
						}
					}
				}

				// Blight shouldnt be set when there is a cliff near
				if (id == map->terrain.blight_texture) {
					if (cliff_near) {
						continue;
					}

					corners[i][j].blight = true;
				} else {
					corners[i][j].blight = false;
					corners[i][j].ground_texture = id;
					corners[i][j].ground_variation = get_random_variation();
				}
				// Set blight pathing
				for (int k = 0; k < 4; k++) {
					for (int l = 0; l < 4; l++) {
						if (i * 4 + k < 0 || i * 4 + k >= map->pathing_map.width || j * 4 + l < 0 || j * 4 + l >= map->pathing_map.height) {
							continue;
						}
						map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b00100000;
						map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= corners[i][j].blight ? 1 : 0;
					}
				}

				if (apply_tile_pathing) {
					for (int k = -2; k < 2; k++) {
						for (int l = -2; l < 2; l++) {
							if (i * 4 + k < 0 || i * 4 + k >= map->pathing_map.width || j * 4 + l < 0 || j * 4 + l >= map->pathing_map.height) {
								continue;
							}
							if (corners[i + std::clamp(k, -1, 0)][j + std::clamp(l, -1, 0)].cliff) {
								continue;
							}
							map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b00001110;
							map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= map->terrain.pathing_options[tile_id].mask();
						}
					}
				}
			}
		}

		// Update texture variations
		for (int i = area2.x(); i < area2.x() + area2.width(); i++) {
			for (int j = area2.y(); j < area2.y() + area2.height(); j++) {
				map->terrain.ground_texture_list[j * (width - 1) + i] = map->terrain.get_texture_variations(i, j);
				if (corners[i][j].cliff) {
					map->terrain.ground_texture_list[j * (width - 1) + i] |= 0b1000000000000000;
				}
			}
		}

		// Update ground texture data
		const int offset = area2.y() * (width - 1) + area2.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width - 1);
		gl->glTextureSubImage2D(map->terrain.ground_texture_data, 0, area2.x(), area2.y(), area2.width(), area2.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map->terrain.ground_texture_list.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	if (apply_height) {
		std::vector< std::vector <float> > heights(area.width(), std::vector<float>(area.height()));

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				heights[i - area.x()][j - area.y()] = ground_heights[j * width + i];
				if (!contains(i - area.x() - std::min(position.x + 1, 0), j - area.y() - std::min(position.y + 1, 0))) {
					continue;
				}
				const int center_x = area.x() + area.width() * 0.5f;
				const int center_y = area.y() + area.height() * 0.5f;

				switch (deformation_type) {
					case deformation::raise: {
						auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						ground_heights[j * width + i] += std::max(0.0, 1 - distance / size * std::sqrt(2)) * 0.1f;
						break;
					}
					case deformation::lower: {
						auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						ground_heights[j * width + i] -= std::max(0.0, 1 - distance / size * std::sqrt(2)) * 0.1f;
						break;
					}
					case deformation::plateau: {
						if (!brush_hold) {
							deformation_height = ground_heights[center_y * width + center_x];
						}

						ground_heights[j * width + i] = deformation_height;
						break;
					}
					case deformation::ripple:
						break;
					case deformation::smooth: {
						float accumulate = 0;

						QRect acum_area = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, width, height});
						for (int k = acum_area.x(); k < acum_area.right() + 1; k++) {
							for (int l = acum_area.y(); l < acum_area.bottom() + 1; l++) {
								if ((k < i || l < j) && k <= i && k - area.x() >= 0 && l - area.y() >= 0 && k < area.right() + 1 && l < area.bottom() + 1) {
									accumulate += heights[k - area.x()][l - area.y()];
								} else {
									accumulate += ground_heights[l * width + k];
								}
							}
						}
						accumulate -= ground_heights[j * width + i];
						ground_heights[j * width + i] = 0.8 * ground_heights[j * width + i] + 0.2 * (accumulate / (acum_area.width() * acum_area.height() - 1));
						break;
					}
				}
				ground_heights[j * width + i] = std::clamp(ground_heights[j * width + i], -16.f, 15.998f);
				map->terrain.ground_corner_heights[j * width + i] = map->terrain.corner_height(i, j);
			}

		}
		const int offset = area.y() * map->terrain.width + area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		gl->glTextureSubImage2D(map->terrain.ground_corner_height, 0, area.x(), area.y(), area.width(), area.height(), GL_RED, GL_FLOAT, map->terrain.ground_corner_heights.data() + offset);
		gl->glTextureSubImage2D(map->terrain.ground_height, 0, area.x(), area.y(), area.width(), area.height(), GL_RED, GL_FLOAT, map->terrain.ground_heights.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	if (apply_cliff) {
		if (!brush_hold) {
			const int center_x = area.x() + area.width() * 0.5f;
			const int center_y = area.y() + area.height() * 0.5f;
			layer_height = corners[center_x][center_y].layer_height;
			switch (cliff_operation_type) {
				case cliff_operation::shallow_water:
					if (!corners[center_x][center_y].water) {
						layer_height -= 1;
					} else if (map->terrain.corner_water_height(center_x, center_y) > map->terrain.corner_height(center_x, center_y) + 1) {
						layer_height += 1;
					}
					break;
				case cliff_operation::lower1:
					layer_height -= 1;
					break;
				case cliff_operation::lower2:
					layer_height -= 2;
				case cliff_operation::deep_water:
					if (!corners[center_x][center_y].water) {
						layer_height -= 2;
					} else if (map->terrain.corner_water_height(center_x, center_y) < map->terrain.corner_height(center_x, center_y) + 1) {
						layer_height -= 1;
					}
					break;
				case cliff_operation::raise1:
					layer_height += 1;
					break;
				case cliff_operation::raise2:
					layer_height += 2;
					break;
			}
			layer_height = std::clamp(layer_height, 0, 15);
		}

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				const int xx = i - area.x() - std::min(position.x + 1, 0);
				const int yy = j - area.y() - std::min(position.y + 1, 0);
				if (!contains(xx, yy)) {
					continue;
				}

				switch (cliff_operation_type) {
					case cliff_operation::lower1:
					case cliff_operation::lower2:
					case cliff_operation::level:
					case cliff_operation::raise1:
					case cliff_operation::raise2:
						corners[i][j].layer_height = layer_height;
						if (corners[i][j].water) {
							if (enforce_water_height_limits && map->terrain.corner_water_height(i, j) < map->terrain.corner_height(i, j)) {
								corners[i][j].water = false;
								map->terrain.water_exists_data[j * width + i] = false;
							}
						}
						break;
					case cliff_operation::shallow_water:
						if (!corners[i][j].water) {
							corners[i][j].water = true;
							map->terrain.water_exists_data[j * width + i] = true;
				  		}
						corners[i][j].layer_height = layer_height;
						water_heights[j * width + i] = corners[i][j].layer_height - 1;
						break;
					case cliff_operation::deep_water:
						if (!corners[i][j].water) {
							corners[i][j].water = true;
							map->terrain.water_exists_data[j * width + i] = true;
						}
						corners[i][j].layer_height = layer_height;
						water_heights[j * width + i] = corners[i][j].layer_height;
						break;
				}
				// Set water pathing
				for (int k = 0; k < 4; k++) {
					for (int l = 0; l < 4; l++) {
						if (i * 4 + k < 0 || i * 4 + k >= map->pathing_map.width || j * 4 + l < 0 || j * 4 + l >= map->pathing_map.height) {
							continue;
						}

						map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b01000000;
						map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= corners[i][j].water ? 1 : 0;
					}
				}

				check_nearby(x, y, i, j, updated_area);
				map->terrain.ground_corner_heights[j * width + i] = map->terrain.corner_height(i, j);
			}
		}

		// Bounds check
		updated_area = updated_area.intersected({ 0, 0, width - 1, height - 1 });

		// Remove all existing cliff meshes in area
		for (size_t i = map->terrain.cliffs.size(); i-- > 0;) {
			glm::ivec3& pos = map->terrain.cliffs[i];
			if (updated_area.contains(pos.x, pos.y)) {
				map->terrain.cliffs.erase(map->terrain.cliffs.begin() + i);
			}
		}

		// Determine if cliff
		for (int i = updated_area.x(); i <= updated_area.right(); i++) {
			for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
				Corner& bottom_left = map->terrain.corners[i][j];
				Corner& bottom_right = map->terrain.corners[i + 1][j];
				Corner& top_left = map->terrain.corners[i][j + 1];
				Corner& top_right = map->terrain.corners[i + 1][j + 1];

				const bool was_cliff = bottom_left.cliff;

				bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height
					|| bottom_left.layer_height != top_left.layer_height
					|| bottom_left.layer_height != top_right.layer_height;

				if (was_cliff && !bottom_left.cliff) {
					if (apply_cliff_pathing && apply_tile_pathing) {
						for (int k = 0; k < 4; k++) {
							for (int l = 0; l < 4; l++) {
								const int id = map->terrain.corners[i  ][j ].ground_texture;
								map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b00001110;
								map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= map->terrain.pathing_options[map->terrain.tileset_ids[id]].mask();
							}
						}
					}
				}

				if (bottom_left.cliff) {
					bottom_left.cliff_texture = cliff_id;

					// Cliff model path
					const int base = std::min({ bottom_left.layer_height, bottom_right.layer_height, top_left.layer_height, top_right.layer_height });
					std::string file_name = ""s + char('A' + bottom_left.layer_height - base)
						+ char('A' + top_left.layer_height - base)
						+ char('A' + top_right.layer_height - base)
						+ char('A' + bottom_right.layer_height - base);

					if (file_name == "AAAA") {
						continue;
					}

					// Clamp to within max variations
					file_name += std::to_string(std::clamp(bottom_left.cliff_variation, 0, map->terrain.cliff_variations[file_name]));

					map->terrain.cliffs.emplace_back(i, j, map->terrain.path_to_cliff[file_name]);

					if (apply_cliff_pathing) {
						for (int k = 0; k < 4; k++) {
							for (int l = 0; l < 4; l++) {
								map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b00001110;
								map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= 0b00001010;
							}
						}
					}
				}
			}
		}

		QRect tile_area = updated_area.adjusted(-1, -1, 1, 1).intersected({ 0, 0, width - 1, height - 1 });
		QRect corner_area = updated_area.adjusted(-1, -1, 1, 1).intersected({ 0, 0, width, height});

		// Update texture variations
		for (int i = tile_area.x(); i < tile_area.x() + tile_area.width(); i++) {
			for (int j = tile_area.y(); j < tile_area.y() + tile_area.height(); j++) {
				map->terrain.ground_texture_list[j * (width - 1) + i] = map->terrain.get_texture_variations(i, j);
				if (corners[i][j].cliff) {
					map->terrain.ground_texture_list[j * (width - 1) + i] |= 0b1000000000000000;
				}
			}
		}

		int offset = tile_area.y() * (width - 1) + tile_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width - 1);
		gl->glTextureSubImage2D(map->terrain.ground_texture_data, 0, tile_area.x(), tile_area.y(), tile_area.width(), tile_area.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map->terrain.ground_texture_list.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		offset = updated_area.y() * width + updated_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		gl->glTextureSubImage2D(map->terrain.ground_corner_height, 0, updated_area.x(), updated_area.y(), updated_area.width() + 1, updated_area.height() + 1, GL_RED, GL_FLOAT, map->terrain.ground_corner_heights.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		offset = tile_area.y() * width + tile_area.x();
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		gl->glTextureSubImage2D(map->terrain.water_exists, 0, tile_area.x(), tile_area.y(), tile_area.width(), tile_area.height(), GL_RED, GL_UNSIGNED_BYTE, map->terrain.water_exists_data.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
			offset = corner_area.y() * width + corner_area.x();
			gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
			gl->glTextureSubImage2D(map->terrain.water_height, 0, corner_area.x(), corner_area.y(), corner_area.width(), corner_area.height(), GL_RED, GL_FLOAT, map->terrain.water_heights.data() + offset);
			gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}

	}

	QRect pathing_area = QRect(updated_area.x() * 4, updated_area.y() * 4, updated_area.width() * 4, updated_area.height() * 4).adjusted(-2, -2, 2, 2).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });

	// Update pathing data
	const int offset = pathing_area.y() * map->pathing_map.width + pathing_area.x();
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map->pathing_map.width);
	gl->glTextureSubImage2D(map->pathing_map.texture_static, 0, pathing_area.x(), pathing_area.y(), pathing_area.width(), pathing_area.height(), GL_RED_INTEGER, GL_UNSIGNED_BYTE, map->pathing_map.pathing_cells_static.data() + offset);
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	if (apply_height || apply_cliff) {
		map->doodads.update_area(updated_area);
		map->units.update_area(updated_area);
	}

	brush_hold = true;
}

void TerrainBrush::apply_end() {
	brush_hold = false;
}

int TerrainBrush::get_random_variation() const {
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_int_distribution<> dist(0, 570);

	int nr = dist(e2) - 1;

	for (auto&&[variation, chance] : variation_chances) {
		if (nr < chance) {
			return variation;
		}
		nr -= chance;
	}
	static_assert("Didn't hit the list of tile variations");
	return 0;
}