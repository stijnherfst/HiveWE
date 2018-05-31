#include "stdafx.h"

// This whole class needs a bit of a rework


// Make this an iterative function instead to avoid stack overflows
void TerrainBrush::check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const {
	QRect bounds = QRect(i - 1, j - 1, 3, 3).intersected({ 0, 0, map.terrain.width, map.terrain.height });

	for (int k = bounds.x(); k <= bounds.right(); k++) {
		for (int l = bounds.y(); l <= bounds.bottom(); l++) {
			if (k == 0 && l == 0) {
				continue;
			}

			int difference = map.terrain.corners[i][j].layer_height - map.terrain.corners[k][l].layer_height;
			if (std::abs(difference) > 2 && !contains(begx + (k - i), begy + (l - k))) {
				map.terrain.corners[k][l].layer_height = map.terrain.corners[i][j].layer_height - std::clamp(difference, -2, 2);
				map.terrain.ground_corner_heights[l * map.terrain.width + k] = map.terrain.corner_height(k, l);

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
	const int x = position.x + 1;
	const int y = position.y + 1;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, map.terrain.width, map.terrain.height });
	QRect area2 = QRect(x - 1, y - 1, cells + 1, cells + 1).intersected({ 0, 0, map.terrain.width - 1, map.terrain.height - 1 });
	QRect updated_area = area2;

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	auto& corners = map.terrain.corners;

	if (apply_texture) {
		const int id = map.terrain.ground_texture_to_id[tile_id];

		// Update textures
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				if (!contains(i - area.x() - std::min(position.x + 1, 0), j - area.y() - std::min(position.y + 1, 0))) {
					continue;
				}

				bool cliff_near = false;
				for (int k = -1; k < 1 && !cliff_near; k++) {
					for (int l = -1; l < 1 && !cliff_near; l++) {
						if (i + k >= 0 && i + k <= map.terrain.width && j + l >= 0 && j + l <= map.terrain.height) {
							cliff_near = corners[i + k][j + l].cliff;
						}
					}
				}

				// Blight shouldnt be set when there is a cliff near
				if (id == map.terrain.blight_texture) {
					if (cliff_near) {
						continue;
					}

					map.terrain.corners[i][j].blight = true;
				} else {
					map.terrain.corners[i][j].blight = false;
					map.terrain.corners[i][j].ground_texture = id;
					map.terrain.corners[i][j].ground_variation = get_random_variation();
				}

				if (apply_tile_pathing) {
					for (int k = -2; k < 2; k++) {
						for (int l = -2; l < 2; l++) {
							if (i * 4 + k < 0 || i * 4 + k >= map.pathing_map.width || j * 4 + l < 0 || j * 4 + l >= map.pathing_map.height) {
								continue;
							}
							if (corners[i + std::clamp(k, -1, 0)][j + std::clamp(l, -1, 0)].cliff) {
								continue;
							}
							map.pathing_map.pathing_cells[(j * 4 + l) * map.pathing_map.width + i * 4 + k] &= ~0b00001110;
							map.pathing_map.pathing_cells[(j * 4 + l) * map.pathing_map.width + i * 4 + k] |= map.terrain.pathing_options[tile_id].mask();
						}
					}
				}
			}
		}

		// Update texture variations
		for (int i = area2.x(); i < area2.x() + area2.width(); i++) {
			for (int j = area2.y(); j < area2.y() + area2.height(); j++) {
				map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] = map.terrain.get_texture_variations(i, j);
				if (corners[i][j].cliff) {
					map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] |= 0b1000000000000000;
				}
			}
		}

		// Update ground texture data
		const int offset = area2.y() * (map.terrain.width - 1) + area2.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width - 1);
		gl->glTextureSubImage2D(map.terrain.ground_texture_data, 0, area2.x(), area2.y(), area2.width(), area2.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map.terrain.ground_texture_list.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	if (apply_height) {
		std::vector< std::vector <float> > heights(area.width(), std::vector<float>(area.height()));

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				heights[i - area.x()][j - area.y()] = corners[i][j].ground_height;
				if (!contains(i - area.x() - std::min(position.x + 1, 0), j - area.y() - std::min(position.y + 1, 0))) {
					continue;
				}
				switch (deformation_type) {
					case deformation::raise: {
						const int center_x = area.x() + area.width() * 0.5f;
						const int center_y = area.y() + area.height() * 0.5f;
						corners[center_x][center_y].ground_height += 0.025;

						float accumulate = 0;

						QRect acum_area = QRect(i - 1, j - 1, 3, 3).intersected({ 0, 0, map.terrain.width, map.terrain.height });
						for (int k = acum_area.x(); k < acum_area.right() + 1; k++) {
							for (int l = acum_area.y(); l < acum_area.bottom() + 1; l++) {
								if ((k < i || l < j) && k <= i && k - area.x() >= 0 && l - area.y() >= 0 && k < area.right() + 1 && l < area.bottom() + 1) {
									accumulate += heights[k - area.x()][l - area.y()];
								} else {
									accumulate += corners[k][l].ground_height;
								}
							}
						}
						accumulate -= corners[i][j].ground_height;
						corners[i][j].ground_height = 0.8 * corners[i][j].ground_height + 0.2 * (accumulate / (acum_area.width() * acum_area.height() - 1));

						break;
					}
					case deformation::lower:
						corners[i][j].ground_height -= 0.125f;
						break;
					case deformation::plateau: {
						if (!brush_hold) {
							const int center_x = area.x() + area.width() * 0.5f;
							const int center_y = area.y() + area.height() * 0.5f;
							deformation_height = corners[center_x][center_y].ground_height;
						}

						map.terrain.corners[i][j].ground_height = deformation_height;
						break;
					}
					case deformation::ripple:
						break;
					case deformation::smooth: {
						float accumulate = 0;

						QRect acum_area = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, map.terrain.width, map.terrain.height});
						for (int k = acum_area.x(); k < acum_area.right() + 1; k++) {
							for (int l = acum_area.y(); l < acum_area.bottom() + 1; l++) {
								if ((k < i || l < j) && k <= i && k - area.x() >= 0 && l - area.y() >= 0 && k < area.right() + 1 && l < area.bottom() + 1) {
									accumulate += heights[k - area.x()][l - area.y()];
								} else {
									accumulate += corners[k][l].ground_height;
								}
							}
						}
						accumulate -= corners[i][j].ground_height;
						corners[i][j].ground_height = 0.8 * corners[i][j].ground_height + 0.2 * (accumulate / (acum_area.width() * acum_area.height() - 1));
						break;
					}
				}
				map.terrain.ground_heights[j * map.terrain.width + i] = map.terrain.corners[i][j].ground_height;
				map.terrain.ground_corner_heights[j * map.terrain.width + i] = map.terrain.corner_height(i, j);
			}

		}
		const int offset = area.y() * map.terrain.width + area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width);
		gl->glTextureSubImage2D(map.terrain.ground_corner_height, 0, area.x(), area.y(), area.width(), area.height(), GL_RED, GL_FLOAT, map.terrain.ground_corner_heights.data() + offset);
		gl->glTextureSubImage2D(map.terrain.ground_height, 0, area.x(), area.y(), area.width(), area.height(), GL_RED, GL_FLOAT, map.terrain.ground_heights.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	if (apply_cliff) {
		if (!brush_hold) {
			const int center_x = area.x() + area.width() * 0.5f;
			const int center_y = area.y() + area.height() * 0.5f;
			layer_height = corners[center_x][center_y].layer_height;
			switch (cliff_operation_type) {
			case cliff_operation::lower2:
				layer_height -= 2;
				break;
			case cliff_operation::lower1:
				layer_height -= 1;
				break;
			case cliff_operation::raise1:
				layer_height += 1;
				break;
			case cliff_operation::raise2:
				layer_height += 2;
				break;
			case cliff_operation::deep_water:
				layer_height -= 2;
				break;
			case cliff_operation::shallow_water:
				layer_height -= 1;
				break;
			default:
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

				corners[i][j].layer_height = layer_height;
				map.terrain.ground_corner_heights[j * map.terrain.width + i] = map.terrain.corner_height(i, j);
				check_nearby(x, y, i, j, updated_area);
			}
		}
		// Bounds check
		updated_area = updated_area.intersected({ 0, 0, map.terrain.width - 1, map.terrain.height - 1 });

		// Remove all existing cliff meshes in area
		for (int i = map.terrain.cliffs.size(); i-- > 0;) {
			glm::ivec3& pos = map.terrain.cliffs[i];
			if (updated_area.contains(pos.x, pos.y)) {
				map.terrain.cliffs.erase(map.terrain.cliffs.begin() + i);
			}
		}

		// Determine if cliff
		for (int i = updated_area.x(); i <= updated_area.right(); i++) {
			for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
				Corner& bottom_left = map.terrain.corners[i][j];
				Corner& bottom_right = map.terrain.corners[i + 1][j];
				Corner& top_left = map.terrain.corners[i][j + 1];
				Corner& top_right = map.terrain.corners[i + 1][j + 1];

				const bool was_cliff = bottom_left.cliff;

				bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height
					|| bottom_left.layer_height != top_left.layer_height
					|| bottom_left.layer_height != top_right.layer_height;

				if (was_cliff && !bottom_left.cliff) {
					if (apply_tile_pathing) {
						for (int k = 0; k < 4; k++) {
							for (int l = 0; l < 4; l++) {
								const int id = map.terrain.corners[i  ][j ].ground_texture;
								map.pathing_map.pathing_cells[((j  ) * 4 + l) * map.pathing_map.width + (i ) * 4 + k] &= ~0b00001110;
								map.pathing_map.pathing_cells[((j  ) * 4 + l) * map.pathing_map.width + (i ) * 4 + k] |= map.terrain.pathing_options[map.terrain.tileset_ids[id]].mask();
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
					file_name += std::to_string(std::clamp(bottom_left.cliff_variation, 0, map.terrain.cliff_variations[file_name]));

					map.terrain.cliffs.emplace_back(i, j, map.terrain.path_to_cliff[file_name]);

					if (apply_cliff_pathing) {
						for (int k = 0; k < 4; k++) {
							for (int l = 0; l < 4; l++) {
								map.pathing_map.pathing_cells[(j * 4 + l) * map.pathing_map.width + i * 4 + k] &= ~0b00001110;
								map.pathing_map.pathing_cells[(j * 4 + l) * map.pathing_map.width + i * 4 + k] |= 0b00001010;
							}
						}
					}
				}
			}
		}

		QRect variation_area = updated_area.adjusted(-1, -1, 1, 1).intersected({ 0, 0, map.terrain.width - 1, map.terrain.height - 1 });
		// Update texture variations
		for (int i = variation_area.x(); i < variation_area.x() + variation_area.width(); i++) {
			for (int j = variation_area.y(); j < variation_area.y() + variation_area.height(); j++) {
				map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] = map.terrain.get_texture_variations(i, j);
				if (corners[i][j].cliff) {
					map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] |= 0b1000000000000000;
				}
			}
		}

		int offset = variation_area.y() * (map.terrain.width - 1) + variation_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width - 1);
		gl->glTextureSubImage2D(map.terrain.ground_texture_data, 0, variation_area.x(), variation_area.y(), variation_area.width(), variation_area.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map.terrain.ground_texture_list.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		offset = updated_area.y() * map.terrain.width + updated_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width);
		gl->glTextureSubImage2D(map.terrain.ground_corner_height, 0, updated_area.x(), updated_area.y(), updated_area.width(), updated_area.height(), GL_RED, GL_FLOAT, map.terrain.ground_corner_heights.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	if (apply_tile_pathing || apply_cliff_pathing) {
		QRect pathing_area = QRect(updated_area.x() * 4, updated_area.y() * 4, updated_area.width() * 4, updated_area.height() * 4).adjusted(-2, -2, 2, 2).intersected({ 0, 0, map.pathing_map.width, map.pathing_map.height });

		// Update pathing data
		const int offset = pathing_area.y() * map.pathing_map.width + pathing_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.pathing_map.width);
		gl->glTextureSubImage2D(map.pathing_map.pathing_texture, 0, pathing_area.x(), pathing_area.y(), pathing_area.width(), pathing_area.height(), GL_RED_INTEGER, GL_UNSIGNED_BYTE, map.pathing_map.pathing_cells.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	brush_hold = true;
}

void TerrainBrush::apply_end() {
	brush_hold = false;
}

int TerrainBrush::get_random_variation() const {
	std::random_device rd;
	std::mt19937 e2(rd());
	const std::uniform_int_distribution<> dist(0, 570);

	int nr = dist(e2) - 1;

	const int id = map.terrain.ground_texture_to_id[tile_id];

	if (!map.terrain.ground_textures[id]->extended) {
		return nr < 285 ? 0 : 15;
	}

	for (auto&&[variation, chance] : variation_chances) {
		if (nr < chance) {
			return variation;
		}
		nr -= chance;
	}
	static_assert("Didn't hit the list of tile variations");
	return 0;
}