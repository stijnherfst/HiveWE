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

				area.setX(std::min(area.x(), k - 1));
				area.setY(std::min(area.y(), l - 1));
				area.setRight(std::max(area.right(), k));
				area.setBottom(std::max(area.bottom(), l));

				check_nearby(begx, begy, k, l, area);
			}
		}
	}
};

void TerrainBrush::apply_begin() {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;

	const int x = position.x + 1;
	const int y = position.y + 1;
	const int cells = size * 2 + 1;
	const QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, width, height });
	const int center_x = area.x() + area.width() * 0.5f;
	const int center_y = area.y() + area.height() * 0.5f;

	// Setup for undo/redo
	map->terrain_undo.new_undo_group();
	map->terrain.new_undo_group();
	map->pathing_map.new_undo_group();
	texture_height_area = area;
	cliff_area = area;

	if (apply_height) {
		deformation_height = corners[center_x][center_y].height;
	}

	if (apply_cliff) {
		layer_height = corners[center_x][center_y].layer_height;
		switch (cliff_operation_type) {
		case cliff_operation::shallow_water:
			if (!corners[center_x][center_y].water) {
				layer_height -= 1;
			} else if (corners[center_x][center_y].final_water_height() > corners[center_x][center_y].final_ground_height() + 1) {
				layer_height += 1;
			}
			break;
		case cliff_operation::lower1:
			layer_height -= 1;
			break;
		case cliff_operation::lower2:
			layer_height -= 2;
			break;
		case cliff_operation::deep_water:
			if (!corners[center_x][center_y].water) {
				layer_height -= 2;
			} else if (corners[center_x][center_y].final_water_height() < corners[center_x][center_y].final_ground_height() + 1) {
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
}

void TerrainBrush::apply() {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;

	const int x = position.x + 1;
	const int y = position.y + 1;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, width, height });

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

				if (id == map->terrain.blight_texture) {
					// Blight shouldn't be set when there is a cliff near
					if (cliff_near) {
						continue;
					}

					corners[i][j].blight = true;
				} else {
					corners[i][j].blight = false;
					corners[i][j].ground_texture = id;
					corners[i][j].ground_variation = get_random_variation();
				}
			}
		}

		map->terrain.update_ground_textures(area);
		texture_height_area = texture_height_area.united(area);
	}

	if (apply_height) {
		std::vector< std::vector <float> > heights(area.width(), std::vector<float>(area.height()));

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				float new_height = corners[i][j].height;
				heights[i - area.x()][j - area.y()] = new_height;

				if (!contains(i - area.x() - std::min(position.x + 1, 0), j - area.y() - std::min(position.y + 1, 0))) {
					continue;
				}
				const int center_x = area.x() + area.width() * 0.5f;
				const int center_y = area.y() + area.height() * 0.5f;

				switch (deformation_type) {
					case deformation::raise: {
						auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						new_height += std::max(0.0, 1 - distance / size * std::sqrt(2)) * 0.1f;
						break;
					}
					case deformation::lower: {
						auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						new_height -= std::max(0.0, 1 - distance / size * std::sqrt(2)) * 0.1f;
						break;
					}
					case deformation::plateau: {
						new_height = deformation_height;
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
									accumulate += corners[k][l].height;
								}
							}
						}
						accumulate -= new_height;
						new_height = 0.8 * new_height + 0.2 * (accumulate / (acum_area.width() * acum_area.height() - 1));
						break;
					}
				}

				corners[i][j].height = std::clamp(new_height, -16.f, 15.98f); // ToDo why 15.98?
			}
		}

		map->terrain.update_ground_heights(area);

		texture_height_area = texture_height_area.united(area);
	}

	QRect updated_area = QRect(x - 1, y - 1, cells + 1, cells + 1).intersected({ 0, 0, width - 1, height - 1 });

	if (apply_cliff) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				const int xx = i - area.x() - std::min(position.x + 1, 0);
				const int yy = j - area.y() - std::min(position.y + 1, 0);
				if (!contains(xx, yy)) {
					continue;
				}

				corners[i][j].layer_height = layer_height;

				switch (cliff_operation_type) {
					case cliff_operation::lower1:
					case cliff_operation::lower2:
					case cliff_operation::level:
					case cliff_operation::raise1:
					case cliff_operation::raise2:
						if (corners[i][j].water) {
							if (enforce_water_height_limits && corners[i][j].final_water_height() < corners[i][j].final_ground_height()) {
								corners[i][j].water = false;
							}
						}
						break;
					case cliff_operation::shallow_water:
						corners[i][j].water = true;
						corners[i][j].water_height = corners[i][j].layer_height - 1;
						break;
					case cliff_operation::deep_water:
						corners[i][j].water = true;
						corners[i][j].water_height = corners[i][j].layer_height;
						break;
				}

				check_nearby(x, y, i, j, updated_area);
			}
		}

		// Bounds check
		updated_area = updated_area.intersected({ 0, 0, width - 1, height - 1 });

		// Determine if cliff
		for (int i = updated_area.x(); i <= updated_area.right(); i++) {
			for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
				Corner& bottom_left = map->terrain.corners[i][j];
				Corner& bottom_right = map->terrain.corners[i + 1][j];
				Corner& top_left = map->terrain.corners[i][j + 1];
				Corner& top_right = map->terrain.corners[i + 1][j + 1];

				bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height
					|| bottom_left.layer_height != top_left.layer_height
					|| bottom_left.layer_height != top_right.layer_height;


				bottom_left.cliff_texture = cliff_id;
			}
		}

		QRect tile_area = updated_area.adjusted(-1, -1, 1, 1).intersected({ 0, 0, width - 1, height - 1 });

		map->terrain.update_cliff_meshes(updated_area);
		map->terrain.update_ground_textures(updated_area);
		map->terrain.update_ground_heights(updated_area.adjusted(0, 0, 1, 1));
		map->terrain.update_water(tile_area.adjusted(0, 0, 1, 1));

		cliff_area = cliff_area.united(updated_area);

		if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
			map->terrain.upload_water_heights();
		}
	}

	// Apply pathing
	for (int i = updated_area.x(); i <= updated_area.right(); i++) {
		for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
			Corner& bottom_left = map->terrain.corners[i][j];

			for (int k = 0; k < 4; k++) {
				for (int l = 0; l < 4; l++) {
					map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] &= ~0b01001110;

					uint8_t mask = 0;
					if (bottom_left.cliff) {
						mask = 0b00001010;
					} else {
						Corner& corner = map->terrain.corners[i + k / 3][j + l / 3];
						const int id = corner.ground_texture;
						mask |= map->terrain.pathing_options[map->terrain.tileset_ids[id]].mask();

						if (corner.water) {
							mask |= 0b01000000;
							if (corner.final_water_height() > corner.final_ground_height() + 0.40) {
								mask |= 0b00001010;
							} else if (corner.final_water_height() > corner.final_ground_height()) {
								mask |= 0b00001000;
							}
						}
					}
					map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= mask;
				}
			}
		}
	}

	map->pathing_map.upload_static_pathing();

	if (apply_height || apply_cliff) {
		if (change_doodad_heights) {
			map->doodads.update_area(updated_area);
		}
		map->units.update_area(updated_area);
	}
}

void TerrainBrush::apply_end() {
	if (apply_texture) {
		map->terrain.add_undo(texture_height_area, Terrain::undo_type::texture);
	}

	if (apply_height) {
		map->terrain.add_undo(texture_height_area, Terrain::undo_type::height);
	}

	if (apply_cliff) {
		QRect cliff_areaa = cliff_area.adjusted(0, 0, 1, 1).intersected({ 0, 0, map->terrain.width, map->terrain.height });
		map->terrain.add_undo(cliff_areaa, Terrain::undo_type::cliff);
	}

	QRect pathing_area = QRect(cliff_area.x() * 4, cliff_area.y() * 4, cliff_area.width() * 4, cliff_area.height() * 4).adjusted(-2, -2, 2, 2).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });
	map->pathing_map.add_undo(pathing_area);

	map->terrain.update_minimap();
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
	assert("Didn't hit the list of tile variations");
	return 0;
}