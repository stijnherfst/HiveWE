#include "terrain_brush.h"
#include "terrain_operators.h"

import std;
import MapGlobal;
import Terrain;
import DoodadsUndo;
import PathingUndo;
import TerrainUndo;
import Camera;

void TerrainOperator::set_brush_type(brush_type brush_type) {
	if (brush_type == brush_type::cell) {
		center_on_tile_corner = false;
	} else if (brush_type == brush_type::corner) {
		center_on_tile_corner = true;
	}

	if (is_active) {
		brush->center_on_tile_corner = center_on_tile_corner;
	}
}

void HeightOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	deformation_height_ground = map->terrain.corners[center_x][center_y].height;
	deformation_height_water = map->terrain.corners[center_x][center_y].water_height;
}

QRect HeightOperator::apply(const QRect& area, double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();

	std::vector<std::vector<float>> heights(area.width(), std::vector<float>(area.height()));
	std::vector<std::vector<float>> water_heights(area.width(), std::vector<float>(area.height()));

	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			float new_height_ground = corners[i][j].height;
			float new_height_water = corners[i][j].water_height;

			heights[i - area.x()][j - area.y()] = new_height_ground;
			water_heights[i - area.x()][j - area.y()] = new_height_water;

			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}
			const int center_x = area.x() + area.width() * 0.5f;
			const int center_y = area.y() + area.height() * 0.5f;

			switch (deformation_type) {
				case deformation::raise: {
					double distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
					double delta = std::max(0.0, 1 - distance / brush->size.x * std::sqrt(2)) * frame_delta;
					new_height_ground += delta;
					new_height_water += delta;
					break;
				}
				case deformation::lower: {
					double distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
					double delta = std::max(0.0, 1 - distance / brush->size.x * std::sqrt(2)) * frame_delta;
					new_height_ground -= delta;
					new_height_water -= delta;
					break;
				}
				case deformation::plateau: {
					new_height_ground = deformation_height_ground;
					new_height_water = deformation_height_water;
					break;
				}
				case deformation::ripple:
					break;
				case deformation::smooth: {
					auto smooth_height = [&](int i, int j, float current_height, auto height_vector, auto get_corner_height) {
						float accumulate = 0;

						QRect acum_area = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, width, height});

						for (int k = acum_area.x(); k < acum_area.right() + 1; k++) {
							for (int l = acum_area.y(); l < acum_area.bottom() + 1; l++) {
								if ((k < i || l < j) && k <= i &&
									// The checks below are required because the code is just wrong (area is too small so we cant convolute a cell from at the edges of the area)
									k - area.x() >= 0 && l - area.y() >= 0 && k < area.right() + 1 && l < area.bottom() + 1) {
									accumulate += height_vector[k - area.x()][l - area.y()];
								} else {
									accumulate += get_corner_height(k, l);
								}
							}
						}
						accumulate -= current_height;
						return 0.8f * current_height + 0.2f * (accumulate / (acum_area.width() * acum_area.height() - 1));
					};

					new_height_ground = smooth_height(i, j, new_height_ground, heights, [&](int k, int l) {
						return corners[k][l].height;
					});

					new_height_water = smooth_height(i, j, new_height_water, water_heights, [&](int k, int l) {
						return corners[k][l].water_height;
					});
					break;
				}
			}

			// ToDo why 15.98?
			if (brush->deform_ground) {
				corners[i][j].height = std::clamp(new_height_ground, -16.f, 15.98f);
			}

			if (brush->deform_water) {
				corners[i][j].water_height = std::clamp(new_height_water, -16.f, 15.98f);
			}
		}
	}

	if (brush->deform_ground) {
		map->terrain.update_ground_heights(area);
	}

	if (brush->deform_water) {
		map->terrain.update_water(area.adjusted(0, 0, 1, 1));
	}

	QRect modified_area = QRect(area.x() * 4, area.y() * 4, area.width() * 4, area.height() * 4);
	return modified_area;
}

void HeightOperator::apply_end() {
	QRect area_terrain = TerrainBrush::from_pathing_rect(brush->updated_area);

	if (brush->deform_ground) {
		brush->add_terrain_undo(area_terrain, TerrainUndoType::height);
	}

	if (brush->deform_water) {
		brush->add_terrain_undo(area_terrain, TerrainUndoType::water);
	}
}

bool HeightOperator::can_combine_with(TerrainOperator* other) {
	// height operator is incompatible with every other operator
	// (vanilla WE behaviour)
	return false;
}

void TextureOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	// nothing to do here, texture operator is simple
}

QRect TextureOperator::apply(const QRect& area, double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();

	// get the tile texture id, do nothing if doesn't exist
	auto it = map->terrain.ground_texture_to_id.find(tile_id);
	if (it == map->terrain.ground_texture_to_id.end()) {
		return QRect();
	}
	const int id = it->second;

	// Update textures
	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}

			if (id == map->terrain.blight_texture) {
				// Blight shouldn't be set when there is a cliff near
				bool cliff_near = false;
				for (int k = -1; k <= 1; k++) {
					for (int l = -1; l <= 1; l++) {
						if (i + k >= 0 && i + k <= width && j + l >= 0 && j + l <= height) {
							cliff_near = cliff_near || corners[i + k][j + l].cliff;
						}
					}
				}

				if (cliff_near) {
					continue;
				}

				corners[i][j].blight = true;
			} else {
				corners[i][j].blight = false;
				corners[i][j].ground_texture = id;
				corners[i][j].set_random_variation();
			}
		}
	}

	map->terrain.update_ground_textures(area);

	// modified area (in pathing resolution)
	QRect modified_area = QRect(area.x() * 4 - 2, area.y() * 4 - 2, area.width() * 4, area.height() * 4);
	return modified_area;
}

void TextureOperator::apply_end() {
	brush->add_terrain_undo(TerrainBrush::from_pathing_rect(brush->updated_area), TerrainUndoType::texture);
}

bool TextureOperator::can_combine_with(TerrainOperator* other) {
	// texture operator can be combined with cliff operator
	if (dynamic_cast<CliffOperator*>(other)) {
		return true;
	}
	return false;
}

void CliffOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	auto& corners = map->terrain.corners;
	layer_height = corners[center_x][center_y].layer_height;
	switch (cliff_operation_type) {
		case cliff_operation::shallow_water:
			if (!corners[center_x][center_y].water) {
				layer_height -= 1;
			} else if (
				corners[center_x][center_y].final_water_height(map->terrain.water_offset)
				> corners[center_x][center_y].final_ground_height() + 1
			) {
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
			} else if (
				corners[center_x][center_y].final_water_height(map->terrain.water_offset)
				< corners[center_x][center_y].final_ground_height() + 1
			) {
				layer_height -= 1;
			}
			break;
		case cliff_operation::raise1:
			layer_height += 1;
			break;
		case cliff_operation::raise2:
			layer_height += 2;
			break;
		case cliff_operation::ramp:
			break;
		case cliff_operation::level:
			break;
	}
	layer_height = std::clamp(layer_height, 0, 15);
}

QRect CliffOperator::apply_cliffs(const QRect& area, double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();
	const glm::vec3 mouse_pos = input_handler.mouse_world;
	const glm::ivec2 pos = mouse_pos - brush->size.x / 4.f / 2.f + 1.f;

	QRect expanded_area = QRect(area.x() - 1, area.y() - 1, area.width() + 1, area.height() + 1).intersected({0, 0, width - 1, height - 1});

	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
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
						if (brush->enforce_water_height_limits
							&& corners[i][j].final_water_height(map->terrain.water_offset) < corners[i][j].final_ground_height()) {
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

			check_nearby(pos.x, pos.y, i, j, expanded_area);
		}
	}
	//}

	// Bounds check
	expanded_area = expanded_area.intersected({0, 0, width - 1, height - 1});

	// Determine if cliff
	for (int i = expanded_area.x(); i <= expanded_area.right(); i++) {
		for (int j = expanded_area.y(); j <= expanded_area.bottom(); j++) {
			Corner& bottom_left = map->terrain.corners[i][j];
			Corner& bottom_right = map->terrain.corners[i + 1][j];
			Corner& top_left = map->terrain.corners[i][j + 1];
			Corner& top_right = map->terrain.corners[i + 1][j + 1];

			bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height || bottom_left.layer_height != top_left.layer_height
				|| bottom_left.layer_height != top_right.layer_height;

			// assign proper cliff texture
			bottom_left.cliff_texture = cliff_id;

			// placing cliffs should delete blight
			if (bottom_left.cliff) {
				bottom_left.blight = false;
				bottom_right.blight = false;
				top_left.blight = false;
				top_right.blight = false;
			}
		}
	}

	QRect tile_area = expanded_area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});

	map->terrain.update_cliff_meshes(tile_area);
	map->terrain.update_ground_textures(expanded_area);
	map->terrain.update_ground_heights(expanded_area.adjusted(0, 0, 1, 1));
	map->terrain.update_water(tile_area.adjusted(0, 0, 1, 1));

	if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
		map->terrain.upload_water_heights();
	}

	// modified area (in pathing resolution)
	QRect modified_area = QRect(expanded_area.x() * 4, expanded_area.y() * 4, expanded_area.width() * 4, expanded_area.height() * 4);
	return modified_area;
}

QRect CliffOperator::apply_ramps(const QRect& area, double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();
	const glm::vec3 mouse_pos = input_handler.mouse_world;
	const glm::ivec2 pos = mouse_pos - brush->size.x / 4.f / 2.f + 1.f;

	QRect modified_area = area;

	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}

			// create new ramps if possible
			int horizontal = (mouse_pos.x > i) - (mouse_pos.x < i);
			int vertical = (mouse_pos.y > j) - (mouse_pos.y < j);
			update_ramp(i, j, horizontal, vertical, modified_area);
		}
	}

	// apply the changes in viewport
	QRect viewport_area = modified_area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});
	map->terrain.update_cliff_meshes(viewport_area);
	map->terrain.update_ground_textures(viewport_area);
	map->terrain.update_ground_heights(viewport_area);

	// convert to pathing resolution
	modified_area =
		QRect(modified_area.x() * 4 - 4, modified_area.y() * 4 - 4, modified_area.width() * 4 + 4, modified_area.height() * 4 + 4);

	return modified_area;
}

QRect CliffOperator::apply(const QRect& area, double frame_delta) {
	if (cliff_operation_type == cliff_operation::ramp) {
		return apply_ramps(area, frame_delta);
	} else {
		return apply_cliffs(area, frame_delta);
	}
}

void CliffOperator::apply_end() {
	brush->add_terrain_undo(TerrainBrush::from_pathing_rect(brush->updated_area), TerrainUndoType::cliff);
}

bool CliffOperator::can_combine_with(TerrainOperator* other) {
	// cliff operator can be combined with texture operator
	if (dynamic_cast<TextureOperator*>(other)) {
		return true;
	}
	return false;
}

/// Make this an iterative function instead to avoid stack overflows
void CliffOperator::check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const {
	QRect bounds = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, map->terrain.width, map->terrain.height});

	for (int k = bounds.x(); k <= bounds.right(); k++) {
		for (int l = bounds.y(); l <= bounds.bottom(); l++) {
			if (k == 0 && l == 0) {
				continue;
			}

			int difference = map->terrain.corners[i][j].layer_height - map->terrain.corners[k][l].layer_height;
			if (std::abs(difference) > 2 && !brush->contains(glm::ivec2(begx + (k - i), begy + (l - k)))) {
				map->terrain.corners[k][l].layer_height = map->terrain.corners[i][j].layer_height - std::clamp(difference, -2, 2);
				map->terrain.corners[k][l].ramp = false;

				area.setX(std::min(area.x(), k - 1));
				area.setY(std::min(area.y(), l - 1));
				area.setRight(std::max(area.right(), k));
				area.setBottom(std::max(area.bottom(), l));

				check_nearby(begx, begy, k, l, area);
			}
		}
	}
}

void CliffOperator::update_ramp(const int i, const int j, int horizontal, int vertical, QRect& rect) {
	// note: this function expects that horizontal and vertical are -1, 0 or 1
	auto& corners = map->terrain.corners;
	int width = map->terrain.width;
	int height = map->terrain.height;

	int origin_level = corners[i][j].layer_height;
	int target_level = origin_level - 1;
	int cliff_tex = corners[i][j].cliff_texture;

	bool allow_ramp_horizontal = horizontal != 0;
	bool allow_ramp_vertical = vertical != 0;
	bool allow_ramp_diagonal = allow_ramp_horizontal && allow_ramp_vertical;

	// lambda which checks if a corner is valid to place a ramp
	auto valid = [&](int x, int y) {
		return x >= 0 && x < width && y >= 0 && y < height && corners[x][y].layer_height == target_level;
	};

	// lambda to check ramp placement in a given direction, returns bool
	auto check_ramp_direction = [&](int dir_x, int dir_y) -> bool {
		// bounds check - ramps take 3 corners
		if (i + 2 * dir_x < 0 || i + 2 * dir_x > width || j + 2 * dir_y < 0 || j + 2 * dir_y > height) {
			return false;
		}

		// check if all 3 corners are valid
		for (int step = 1; step <= 2; ++step) {
			if (!valid(i + step * dir_x, j + step * dir_y)) {
				return false;
			}
		}

		// check perpendicular neighbours - if they are higher, the ramp cannot be placed
		if (corners[i + dir_y][j + dir_x].layer_height > origin_level || corners[i - dir_y][j - dir_x].layer_height > origin_level) {
			return false;
		}

		// it is illegal to place a ramp right next to the cliff edge which contains
		// ONLY the ramp in the opposite direction (horizontal vs vertical)
		for (int side : {-1, 1}) {
			if (corners[i][j + side].ramp
				&& (!corners[i + dir_x][j + side + dir_y].ramp || !corners[i + 2 * dir_x][j + side + 2 * dir_y].ramp)) {
				return false;
			}

			if (corners[i + side][j].ramp
				&& (!corners[i + side + dir_x][j + dir_y].ramp || !corners[i + side + 2 * dir_x][j + 2 * dir_y].ramp)) {
				return false;
			}
		}

		return true;
	};

	// check horizontal ramp
	allow_ramp_horizontal = check_ramp_direction(horizontal, 0);

	// check vertical ramp
	allow_ramp_vertical = check_ramp_direction(0, vertical);

	// check if we can place a diagonal ramp
	for (int dx = 0; dx <= 2 && allow_ramp_diagonal; ++dx) {
		for (int dy = 0; dy <= 2 && allow_ramp_diagonal; ++dy) {
			if (dx == 0 && dy == 0) {
				continue;
			}

			if (!valid(i + dx * horizontal, j + dy * vertical)) {
				allow_ramp_diagonal = false;
			}
		}
	}

	// place valid ramps
	if (allow_ramp_horizontal) {
		for (int step = 0; step <= 2; ++step) {
			corners[i + step * horizontal][j].ramp = true;
			corners[i + step * horizontal][j].cliff_texture = cliff_tex;
		}
	}

	if (allow_ramp_vertical) {
		for (int step = 0; step <= 2; ++step) {
			corners[i][j + step * vertical].ramp = true;
			corners[i][j + step * vertical].cliff_texture = cliff_tex;
		}
	}

	if (allow_ramp_diagonal) {
		for (int dx = 0; dx <= 2; ++dx) {
			for (int dy = 0; dy <= 2; ++dy) {
				corners[i + dx * horizontal][j + dy * vertical].ramp = true;
				corners[i + dx * horizontal][j + dy * vertical].cliff_texture = cliff_tex;
			}
		}
	}

	// add missing center-piece ramp flags at all possible L-corners
	if (allow_ramp_horizontal || allow_ramp_diagonal) {
		if (valid(i + horizontal, j + 1) && corners[i][j].ramp && corners[i + 1][j].ramp && corners[i + 2][j].ramp && corners[i][j + 1].ramp
			&& corners[i][j + 2].ramp) {
			corners[i + horizontal][j + 1].ramp = true;
		}

		if (valid(i + horizontal, j - 1) && corners[i][j].ramp && corners[i + 1][j].ramp && corners[i + 2][j].ramp && corners[i][j - 1].ramp
			&& corners[i][j - 2].ramp) {
			corners[i + horizontal][j - 1].ramp = true;
		}
	}

	if (allow_ramp_vertical || allow_ramp_diagonal) {
		if (valid(i + 1, j + vertical) && corners[i][j].ramp && corners[i][j + 1].ramp && corners[i][j + 2].ramp && corners[i + 1][j].ramp
			&& corners[i + 2][j].ramp) {
			corners[i + 1][j + vertical].ramp = true;
		}

		if (valid(i - 1, j + vertical) && corners[i][j].ramp && corners[i][j + 1].ramp && corners[i][j + 2].ramp && corners[i - 1][j].ramp
			&& corners[i - 2][j].ramp) {
			corners[i - 1][j + vertical].ramp = true;
		}
	}

	// update the rect to include the newest ramps
	rect = rect.adjusted(
		(allow_ramp_horizontal && horizontal < 0 || allow_ramp_diagonal && horizontal < 0) ? -2 : 0,
		(allow_ramp_vertical && vertical < 0 || allow_ramp_diagonal && vertical < 0) ? -2 : 0,
		(allow_ramp_horizontal && horizontal > 0 || allow_ramp_diagonal && horizontal > 0) ? 2 : 0,
		(allow_ramp_vertical && vertical > 0 || allow_ramp_diagonal && vertical > 0) ? 2 : 0
	);
}

void CellOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	Corner& corner = map->terrain.corners[center_x][center_y];
	int terrain_height = corner.layer_height - 2 + corner.height;
	water_height = terrain_height + CellOperator::WATER_GROUND_ZERO + CellOperator::WATER_HEIGHT;
}

QRect CellOperator::apply(const QRect& area, double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();

	bool edits_water = cell_operation_type == cell_operation::remove_water || cell_operation_type == cell_operation::add_water;

	// note: cell operator brush targets cells, not corners
	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}

			// bounds check
			if (i >= width || j >= width || i < 0 || j < 0) {
				continue;
			}

			// (i, j) is the bottom-left corner here
			if (cell_operation_type == cell_operation::add_water) {
				if (!corners[i][j].water) {
					corners[i][j].water = true;
					corners[i][j].water_height = water_height;
				} else {
					// if the water is not visible (below the ground), we want to incerase it's height
					int terrain_height = corners[i][j].layer_height - 2 + corners[i][j].height;
					if (corners[i][j].water_height <= terrain_height + CellOperator::WATER_GROUND_ZERO) {
						corners[i][j].water_height = water_height;
					}
				}
			} else if (cell_operation_type == cell_operation::remove_water) {
				corners[i][j].water = false;
				corners[i][j].water_height = 0;
			} else if (cell_operation_type == cell_operation::add_boundary) {
				corners[i][j].boundary = true;
			} else if (cell_operation_type == cell_operation::remove_boundary) {
				corners[i][j].boundary = false;
			} else if (cell_operation_type == cell_operation::add_hole) {
				// future work
			} else if (cell_operation_type == cell_operation::remove_hole) {
				// future work
			}
		}
	}

	// finally, re-render the water if we changed it
	if (edits_water) {
		map->terrain.update_water(area.adjusted(0, 0, 1, 1));
	}

	// modified area (in pathing resolution)
	QRect modified_area;
	if (brush->center_on_tile_corner) {
		modified_area = QRect(area.x() * 4 - 2, area.y() * 4 - 2, area.width() * 4, area.height() * 4);
	} else {
		modified_area = QRect(area.x() * 4, area.y() * 4, area.width() * 4, area.height() * 4);
	}
	return modified_area;
}

void CellOperator::apply_end() {
	QRect area_terrain = TerrainBrush::from_pathing_rect(brush->updated_area);

	if (cell_operation_type == cell_operation::remove_water || cell_operation_type == cell_operation::add_water) {
		brush->add_terrain_undo(area_terrain, TerrainUndoType::water);
	} else if (cell_operation_type == cell_operation::add_boundary || cell_operation_type == cell_operation::remove_boundary) {
		brush->add_terrain_undo(area_terrain, TerrainUndoType::texture);
	} else if (cell_operation_type == cell_operation::add_hole || cell_operation_type == cell_operation::remove_hole) {
		brush->add_terrain_undo(area_terrain, TerrainUndoType::cliff);
	}
}

bool CellOperator::can_combine_with(TerrainOperator* other) {
	return false;
}
