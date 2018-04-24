#include "stdafx.h"



void TerrainBrush::apply() {
	const int x = position.x + brush_offset.x + 1;
	const int y = position.y + brush_offset.y + 1;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, map.terrain.width, map.terrain.height });
	QRect area2 = QRect(x - 1, y - 1, cells + 1, cells + 1).intersected({ 0, 0, map.terrain.width - 1, map.terrain.height - 1});

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	if (apply_texture) {
		const int id = map.terrain.ground_texture_to_id[tile_id];

		// Update textures
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				// Blight shouldnt be set when there is a cliff near
				if (id == map.terrain.blight_texture) {
					for (int k = -1; k < 1; k++) {
						for (int l = -1; l < 1; l++) {
							if (i + k >= 0 && i + k <= map.terrain.width && j + l >= 0 && j + l <= map.terrain.height) {
								if (map.terrain.corners[i + k][j + l].cliff) {
									goto exitloop;
								}
							}
						}
					}

					map.terrain.corners[i][j].blight = true;
				} else {
					map.terrain.corners[i][j].blight = false;
					map.terrain.corners[i][j].ground_texture = id;
					map.terrain.corners[i][j].ground_variation = get_random_variation();
				}
			exitloop:;
			}
		}

		// Update texture variations
		for (int i = area2.x(); i < area2.x() + area2.width(); i++) {
			for (int j = area2.y(); j < area2.y() + area2.height(); j++) {
				map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] = map.terrain.get_texture_variations(i, j);
				if (map.terrain.corners[i][j].cliff) {
					map.terrain.ground_texture_list[j * (map.terrain.width - 1) + i] |= 0b1000000000000000;
				}
			}
		}

		// Set tile pathing
		uint8_t mask = 0;
		if (map.terrain.pathing_options[tile_id].unwalkable) {
			mask |= 0b00000010;
		}
		if (map.terrain.pathing_options[tile_id].unflyable) {
			mask |= 0b00000100;
		}
		if (map.terrain.pathing_options[tile_id].unbuildable) {
			mask |= 0b00001000;
		}

		QRect pathing_area = QRect(x * 4 - 2, y * 4 - 2, cells * 4, cells * 4).intersected({ 0, 0, map.pathing_map.width, map.pathing_map.height });

		for (int i = pathing_area.x(); i < pathing_area.x() + pathing_area.width(); i++) {
			for (int j = pathing_area.y(); j < pathing_area.y() + pathing_area.height(); j++) {
				uint8_t byte_cell = map.pathing_map.pathing_cells[j * map.pathing_map.width + i];

				byte_cell &= ~0b00001110;
				byte_cell |= mask;

				map.pathing_map.pathing_cells[j * map.pathing_map.width + i] = byte_cell;
			}
		}

		// Update pathing data
		int offset = pathing_area.y() * map.pathing_map.width + pathing_area.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.pathing_map.width);
		gl->glTextureSubImage2D(map.pathing_map.pathing_texture, 0, pathing_area.x(), pathing_area.y(), pathing_area.width(), pathing_area.height(), GL_RED_INTEGER, GL_UNSIGNED_BYTE, map.pathing_map.pathing_cells.data() + offset);

		// Update ground texture data
		offset = area2.y() * (map.terrain.width - 1) + area2.x();
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width - 1);
		gl->glTextureSubImage2D(map.terrain.ground_texture_data, 0, area2.x(), area2.y(), area2.width(), area2.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map.terrain.ground_texture_list.data() + offset);
		gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	if (apply_height) {
		std::vector< std::vector <float> > heights(area.width(), std::vector<float>(area.height()));

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				heights[i - area.x()][j - area.y()] = map.terrain.corners[i][j].ground_height;
				switch (deformation_type) {
					case deformation::raise:
						map.terrain.corners[i][j].ground_height += 0.125f;
						break;
					case deformation::lower:
						map.terrain.corners[i][j].ground_height -= 0.125f;
						break;
					case deformation::plateau: {
						const int center_x = area.x() + area.width() * 0.5f;
						const int center_y = area.y() + area.height() * 0.5f;

						map.terrain.corners[i][j].ground_height = map.terrain.corners[center_x][center_y].ground_height;
						break;
					}
					case deformation::ripple:
						break;
					case deformation::smooth: {
						int counts = 0;
						float accumulate = 0;
						for (int k = -1; k < 1; k++) {
							if (i + k < 0 || i + k > map.terrain.width) {
								continue;
							}
							for (int l = -1; l < 1; l++) {
								if (j + l < 0 || j + l > map.terrain.height || (k == 0 && l == 0)) {
									continue;
								}

								counts++;
								if ((k == -1 || l == -1) && i - area.x() > 0 && j - area.y() > 0) {
									int t = i - area.x();
									accumulate += heights[i - area.x() + k][j - area.y() + l];
								} else {
									accumulate += map.terrain.corners[i][j].ground_height;
								}
							}
						}
						
						map.terrain.corners[i][j].ground_height += accumulate / counts;
						map.terrain.corners[i][j].ground_height *= 0.5;
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
}

//void TerrainBrush::apply_texture() {

//}

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
	static_assert("Should never come here");
	return 0;
}
