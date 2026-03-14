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

void HeightOperator::apply(const QRect& area, double frame_delta, QRect& updated_area) {
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

	brush->texture_height_area = brush->texture_height_area.united(area);
}

void HeightOperator::apply_end() {
	if (brush->deform_ground) {
		brush->add_terrain_undo(brush->texture_height_area, TerrainUndoType::height);
	}

	if (brush->deform_water) {
		brush->add_terrain_undo(brush->texture_height_area, TerrainUndoType::water);
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

void TextureOperator::apply(const QRect& area, double frame_delta, QRect& updated_area) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();

	// get the tile texture id, do nothing if doesn't exist
	auto it = map->terrain.ground_texture_to_id.find(tile_id);
	if (it == map->terrain.ground_texture_to_id.end()) {
		return;
	}
	const int id = it->second;

	// Update textures
	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}

			bool cliff_near = false;
			for (int k = -1; k < 1; k++) {
				for (int l = -1; l < 1; l++) {
					if (i + k >= 0 && i + k <= width && j + l >= 0 && j + l <= height) {
						cliff_near = cliff_near || corners[i + k][j + l].cliff;
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
				corners[i][j].set_random_variation();
			}
		}
	}

	map->terrain.update_ground_textures(area);
	brush->texture_height_area = brush->texture_height_area.united(area);
}

void TextureOperator::apply_end() {
	brush->add_terrain_undo(brush->texture_height_area, TerrainUndoType::texture);
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

void CliffOperator::apply(const QRect& area, double frame_delta, QRect& updated_area) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& corners = map->terrain.corners;
	const glm::vec2 position = brush->get_position();
	const glm::ivec2 pos = glm::vec2(input_handler.mouse_world) - brush->size.x / 4.f / 2.f + 1.f;

	//if (cliff_operation_type == cliff_operation::ramp) {
	//	const int center_x = area.x() + area.width() * 0.5f;
	//	const int center_y = area.y() + area.height() * 0.5f;

	//	glm::vec2 p = glm::vec2(input_handler.mouse_world) - get_position();

	//	int cliff_count = corners[center_x][center_y].cliff + corners[center_x - 1][center_y].cliff + corners[center_x][center_y - 1].cliff + corners[center_x - 1][center_y - 1].cliff;

	//	// Cliff count 1 and 4 are nothing

	//	if (cliff_count == 2 ) {
	//		corners[center_x][center_y].ramp = true;

	//		// possibly place a new ramp
	//	} else if (cliff_count == 3) {
	//		// Target for whole rampification
	//	}

	//	std::cout << cliff_count << "\n";

	//	if (corners[center_x - (p.x < 1)][center_y - (p.y < 1)].cliff) {
	//	//	corners[center_x][center_y].ramp = true;
	//	//	std::cout << "Ramp set\n";
	//	}

	//	//if (corners[center_x][center_y].cliff) {
	//	//	corners[i][j].ramp = true;
	//	//}
	//} else {
	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
				continue;
			}
			corners[i][j].ramp = false;
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
				case cliff_operation::ramp:
					break;
			}

			brush->check_nearby(pos.x, pos.y, i, j, updated_area);
		}
	}
	//}

	// Bounds check
	updated_area = updated_area.intersected({0, 0, width - 1, height - 1});

	// Determine if cliff
	for (int i = updated_area.x(); i <= updated_area.right(); i++) {
		for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
			Corner& bottom_left = map->terrain.corners[i][j];
			Corner& bottom_right = map->terrain.corners[i + 1][j];
			Corner& top_left = map->terrain.corners[i][j + 1];
			Corner& top_right = map->terrain.corners[i + 1][j + 1];

			bottom_left.cliff = bottom_left.layer_height != bottom_right.layer_height || bottom_left.layer_height != top_left.layer_height
				|| bottom_left.layer_height != top_right.layer_height;

			if (cliff_operation_type != cliff_operation::ramp) {
				bottom_left.cliff_texture = cliff_id;
			}
		}
	}

	QRect tile_area = updated_area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});

	map->terrain.update_cliff_meshes(tile_area);
	map->terrain.update_ground_textures(updated_area);
	map->terrain.update_ground_heights(updated_area.adjusted(0, 0, 1, 1));
	map->terrain.update_water(tile_area.adjusted(0, 0, 1, 1));

	brush->cliff_area = brush->cliff_area.united(updated_area);

	if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
		map->terrain.upload_water_heights();
	}
}

void CliffOperator::apply_end() {
	QRect undo_area = brush->cliff_area.adjusted(0, 0, 1, 1).intersected({0, 0, map->terrain.width, map->terrain.height});
	brush->add_terrain_undo(undo_area, TerrainUndoType::cliff);
}

bool CliffOperator::can_combine_with(TerrainOperator* other) {
	// cliff operator can be combined with texture operator
	if (dynamic_cast<TextureOperator*>(other)) {
		return true;
	}
	return false;
}

void CellOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	Corner& corner = map->terrain.corners[center_x][center_y];
	int terrain_height = corner.layer_height - 2 + corner.height;
	water_height = terrain_height + CellOperator::WATER_GROUND_ZERO + CellOperator::WATER_HEIGHT;
}

void CellOperator::apply(const QRect& area, double frame_delta, QRect& updated_area) {
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
				corners[i][j].water_height = -1;
			} else if (cell_operation_type == cell_operation::add_boundary) {
				corners[i][j].boundary = true;
			} else if (cell_operation_type == cell_operation::remove_boundary) {
				corners[i][j].boundary = false;
			}
		}
	}

	// finally, re-render the water if we changed it
	if (edits_water) {
		map->terrain.update_water(area.adjusted(0, 0, 1, 1));
	}

	brush->texture_height_area = brush->texture_height_area.united(area);
}

void CellOperator::apply_end() {
	if (cell_operation_type == cell_operation::remove_water || cell_operation_type == cell_operation::add_water) {
		brush->add_terrain_undo(brush->texture_height_area, TerrainUndoType::water);
	}
}

bool CellOperator::can_combine_with(TerrainOperator* other) {
	return false;
}
