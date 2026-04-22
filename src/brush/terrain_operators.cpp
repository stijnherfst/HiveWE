#include "terrain_brush.h"
#include "terrain_operators.h"

import std;
import MapGlobal;
import Terrain;
import DoodadsUndo;
import PathingUndo;
import TerrainUndo;
import WorldUndoManager;
import Camera;

void TerrainOperator::set_brush_type(Brush::Type type) {
	brush_type = type;

	if (is_active) {
		brush->brush_type = type;
	}
}

void HeightOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	auto& terrain = map->terrain;
	const size_t center_idx = terrain.ci(center_x, center_y);
	deformation_height_ground = terrain.corner_height[center_idx];
	deformation_height_water = terrain.corner_water_height[center_idx];
}

QRect HeightOperator::apply(const QRect& area, double frame_delta) {
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const glm::ivec2 pos = brush->get_unclipped_pos();

	const int area_h = area.height();
	std::vector<float> heights(area.width() * area_h);
	std::vector<float> water_heights(area.width() * area_h);

	for (int j = area.y(); j < area.y() + area.height(); j++) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			const size_t idx = terrain.ci(i, j);
			float new_height_ground = terrain.corner_height[idx];
			float new_height_water = terrain.corner_water_height[idx];

			heights[(i - area.x()) * area_h + (j - area.y())] = new_height_ground;
			water_heights[(i - area.x()) * area_h + (j - area.y())] = new_height_water;

			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(pos, 0))) {
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
									accumulate += height_vector[(k - area.x()) * area_h + (l - area.y())];
								} else {
									accumulate += get_corner_height(k, l);
								}
							}
						}
						accumulate -= current_height;
						return 0.8f * current_height + 0.2f * (accumulate / (acum_area.width() * acum_area.height() - 1));
					};

					if (brush->deform_ground) {
						new_height_ground = smooth_height(i, j, new_height_ground, heights, [&](int k, int l) {
							return terrain.corner_height[terrain.ci(k, l)];
						});
					}

					if (brush->deform_water) {
						new_height_water = smooth_height(i, j, new_height_water, water_heights, [&](int k, int l) {
							return terrain.corner_water_height[terrain.ci(k, l)];
						});
					}

					break;
				}
			}

			if (brush->deform_ground) {
				terrain.corner_height[idx] = std::clamp(new_height_ground, Terrain::min_ground_height, Terrain::max_ground_height);
			}

			if (brush->deform_water) {
				terrain.corner_water_height[idx] = std::clamp(new_height_water, Terrain::min_ground_height, Terrain::max_ground_height);
			}
		}
	}

	if (brush->deform_ground) {
		terrain.update_ground_heights(area);
	}

	if (brush->deform_water) {
		terrain.update_water(area.adjusted(0, 0, 1, 1).intersected({0, 0, width, height}));
	}

	QRect modified_area = TerrainBrush::to_pathing_rect(area).adjusted(-2, -2, 2, 2);
	return modified_area;
}

void HeightOperator::apply_end(WorldEditContext& ctx, const QRect& area) {
	QRect area_terrain = TerrainBrush::from_pathing_rect(area);

	if (brush->deform_ground) {
		brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::height);
	}

	if (brush->deform_water) {
		brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::water);
	}
}

void TextureOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	// nothing to do here, texture operator is simple
}

QRect TextureOperator::apply(const QRect& area, double frame_delta) {
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const glm::ivec2 pos = brush->get_unclipped_pos();

	const int id = terrain.ground_texture_to_id.at(tile_id);

	// Update textures
	for (int j = area.y(); j < area.y() + area.height(); j++) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(pos, 0))) {
				continue;
			}

			const size_t idx = terrain.ci(i, j);

			if (id == terrain.blight_texture) {
				// Blight shouldn't be set when there is a cliff near
				bool cliff_near = false;
				for (int k = -1; k <= 1; k++) {
					for (int l = -1; l <= 1; l++) {
						if (i + k >= 0 && i + k <= width && j + l >= 0 && j + l <= height) {
							cliff_near = cliff_near || terrain.corner_cliff[terrain.ci(i + k, j + l)];
						}
					}
				}

				if (cliff_near) {
					continue;
				}

				terrain.corner_blight[idx] = true;
			} else {
				terrain.corner_blight[idx] = false;
				terrain.corner_ground_texture[idx] = id;
				terrain.corner_ground_variation[idx] = random_ground_variation();
			}
		}
	}

	terrain.update_ground_textures(area);

	// modified area (in pathing resolution)
	QRect modified_area = TerrainBrush::to_pathing_rect(area).adjusted(-2, -2, -2, -2);
	return modified_area;
}

void TextureOperator::apply_end(WorldEditContext& ctx, const QRect& area) {
	const QRect area_terrain = TerrainBrush::from_pathing_rect(area);
	brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::texture);
}

void CliffOperator::apply_begin(const QRect& area, int center_x, int center_y) {
	auto& terrain = map->terrain;
	const size_t center_idx = terrain.ci(center_x, center_y);
	layer_height = terrain.corner_layer_height[center_idx];

	switch (cliff_operation_type) {
		case cliff_operation::shallow_water:
			if (!terrain.corner_water[center_idx]) {
				layer_height -= 1;
			} else if (terrain.corner_final_water_height(center_x, center_y) > terrain.corner_final_ground_height(center_x, center_y) + 1) {
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
			if (!terrain.corner_water[center_idx]) {
				layer_height -= 2;
			} else if (terrain.corner_final_water_height(center_x, center_y) < terrain.corner_final_ground_height(center_x, center_y) + 1) {
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
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const glm::ivec2 pos = brush->get_unclipped_pos();
	const glm::vec3 mouse_pos = input_handler.mouse_world;

	QRect expanded_area = QRect(area.x() - 1, area.y() - 1, area.width() + 1, area.height() + 1).intersected({0, 0, width - 1, height - 1});

	for (int j = area.y(); j < area.y() + area.height(); j++) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(pos, 0))) {
				continue;
			}

			const size_t idx = terrain.ci(i, j);
			terrain.corner_ramp[idx] = false;
			terrain.corner_layer_height[idx] = layer_height;

			switch (cliff_operation_type) {
				case cliff_operation::lower1:
				case cliff_operation::lower2:
				case cliff_operation::level:
				case cliff_operation::raise1:
				case cliff_operation::raise2:
					if (terrain.corner_water[idx]) {
						if (brush->enforce_water_height_limits
							&& terrain.corner_final_water_height(i, j) < terrain.corner_final_ground_height(i, j)) {
							terrain.corner_water[idx] = false;
						}
					}
					break;
				case cliff_operation::shallow_water:
					terrain.corner_water[idx] = true;
					terrain.corner_water_height[idx] = terrain.corner_layer_height[idx] - 1;
					break;
				case cliff_operation::deep_water:
					terrain.corner_water[idx] = true;
					terrain.corner_water_height[idx] = terrain.corner_layer_height[idx];
					break;
			}

			check_nearby(pos.x, pos.y, i, j, expanded_area);
		}
	}

	// Bounds check
	expanded_area = expanded_area.intersected({0, 0, width - 1, height - 1});

	// Determine if cliff
	for (size_t i = expanded_area.x(); i <= static_cast<size_t>(expanded_area.right()); i++) {
		for (size_t j = expanded_area.y(); j <= static_cast<size_t>(expanded_area.bottom()); j++) {
			const size_t bl = terrain.ci(i, j);
			const size_t br = terrain.ci(i + 1, j);
			const size_t tl = terrain.ci(i, j + 1);
			const size_t tr = terrain.ci(i + 1, j + 1);

			terrain.corner_cliff[bl] = terrain.corner_layer_height[bl] != terrain.corner_layer_height[br]
				|| terrain.corner_layer_height[bl] != terrain.corner_layer_height[tl]
				|| terrain.corner_layer_height[bl] != terrain.corner_layer_height[tr];

			// assign proper cliff texture
			terrain.corner_cliff_texture[bl] = cliff_id;

			// placing cliffs should delete blight
			if (terrain.corner_cliff[bl]) {
				terrain.corner_blight[bl] = false;
				terrain.corner_blight[br] = false;
				terrain.corner_blight[tl] = false;
				terrain.corner_blight[tr] = false;
			}
		}
	}

	QRect tile_area = expanded_area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});

	terrain.update_cliff_meshes(tile_area);
	terrain.update_ground_textures(expanded_area);
	terrain.update_ground_heights(expanded_area.adjusted(0, 0, 1, 1));
	terrain.update_water(tile_area.adjusted(0, 0, 1, 1));

	if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
		terrain.upload_water_heights();
	}

	// modified area (in pathing resolution)
	QRect modified_area = TerrainBrush::to_pathing_rect(expanded_area);
	return modified_area;
}

QRect CliffOperator::apply_ramps(const QRect& area, double frame_delta) {
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const glm::ivec2 pos = brush->get_unclipped_pos();
	const glm::vec3 mouse_pos = input_handler.mouse_world;

	QRect modified_area = area;

	for (int j = area.y(); j < area.y() + area.height(); j++) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(pos, 0))) {
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
	terrain.update_cliff_meshes(viewport_area);
	terrain.update_ground_textures(viewport_area);
	terrain.update_ground_heights(viewport_area);

	// convert to pathing resolution
	modified_area = TerrainBrush::to_pathing_rect(modified_area).adjusted(-4, -4, 0, 0);

	return modified_area;
}

QRect CliffOperator::apply(const QRect& area, double frame_delta) {
	if (cliff_operation_type == cliff_operation::ramp) {
		return apply_ramps(area, frame_delta);
	} else {
		return apply_cliffs(area, frame_delta);
	}
}

void CliffOperator::apply_end(WorldEditContext& ctx, const QRect& area) {
	brush->add_terrain_undo(ctx, TerrainBrush::from_pathing_rect(area), TerrainUndoType::cliff);
}

/// Make this an iterative function instead to avoid stack overflows
void CliffOperator::check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const {
	auto& terrain = map->terrain;
	QRect bounds = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, terrain.width, terrain.height});

	for (int l = bounds.y(); l <= bounds.bottom(); l++) {
		for (int k = bounds.x(); k <= bounds.right(); k++) {
			if (k == 0 && l == 0) {
				continue;
			}

			int difference = terrain.corner_layer_height[terrain.ci(i, j)] - terrain.corner_layer_height[terrain.ci(k, l)];
			if (std::abs(difference) > 2 && !brush->contains(glm::ivec2(begx + (k - i), begy + (l - k)))) {
				terrain.corner_layer_height[terrain.ci(k, l)] =
					terrain.corner_layer_height[terrain.ci(i, j)] - std::clamp(difference, -2, 2);
				terrain.corner_ramp[terrain.ci(k, l)] = false;

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
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const size_t idx = terrain.ci(i, j);

	int origin_level = terrain.corner_layer_height[idx];
	int target_level = origin_level - 1;
	int cliff_tex = terrain.corner_cliff_texture[idx];

	bool allow_ramp_horizontal = horizontal != 0;
	bool allow_ramp_vertical = vertical != 0;
	bool allow_ramp_diagonal = allow_ramp_horizontal && allow_ramp_vertical;

	// lambda which checks if a corner is valid to place a ramp
	auto valid = [&](int x, int y) {
		return x >= 0 && x < width && y >= 0 && y < height && terrain.corner_layer_height[terrain.ci(x, y)] == target_level;
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
		if (terrain.corner_layer_height[terrain.ci(i + dir_y, j + dir_x)] > origin_level
			|| terrain.corner_layer_height[terrain.ci(i - dir_y, j - dir_x)] > origin_level) {
			return false;
		}

		// it is illegal to place a ramp right next to the cliff edge which contains
		// ONLY the ramp in the opposite direction (horizontal vs vertical)
		for (int side : {-1, 1}) {
			if (terrain.corner_ramp[terrain.ci(i, j + side)]
				&& (!terrain.corner_ramp[terrain.ci(i + dir_x, j + side + dir_y)]
					|| !terrain.corner_ramp[terrain.ci(i + 2 * dir_x, j + side + 2 * dir_y)])) {
				return false;
			}

			if (terrain.corner_ramp[terrain.ci(i + side, j)]
				&& (!terrain.corner_ramp[terrain.ci(i + side + dir_x, j + dir_y)]
					|| !terrain.corner_ramp[terrain.ci(i + side + 2 * dir_x, j + 2 * dir_y)])) {
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
			terrain.corner_ramp[terrain.ci(i + step * horizontal, j)] = true;
			terrain.corner_cliff_texture[terrain.ci(i + step * horizontal, j)] = cliff_tex;
		}
	}

	if (allow_ramp_vertical) {
		for (int step = 0; step <= 2; ++step) {
			terrain.corner_ramp[terrain.ci(i, j + step * vertical)] = true;
			terrain.corner_cliff_texture[terrain.ci(i, j + step * vertical)] = cliff_tex;
		}
	}

	if (allow_ramp_diagonal) {
		for (int dx = 0; dx <= 2; ++dx) {
			for (int dy = 0; dy <= 2; ++dy) {
				terrain.corner_ramp[terrain.ci(i + dx * horizontal, j + dy * vertical)] = true;
				terrain.corner_cliff_texture[terrain.ci(i + dx * horizontal, j + dy * vertical)] = cliff_tex;
			}
		}
	}

	// add missing center-piece ramp flags at all possible L-corners
	if (allow_ramp_horizontal || allow_ramp_diagonal) {
		if (valid(i + horizontal, j + 1) && terrain.corner_ramp[terrain.ci(i, j)] && terrain.corner_ramp[terrain.ci(i + 1, j)]
			&& terrain.corner_ramp[terrain.ci(i + 2, j)] && terrain.corner_ramp[terrain.ci(i, j + 1)]
			&& terrain.corner_ramp[terrain.ci(i, j + 2)]) {
			terrain.corner_ramp[terrain.ci(i + horizontal, j + 1)] = true;
		}

		if (valid(i + horizontal, j - 1) && terrain.corner_ramp[terrain.ci(i, j)] && terrain.corner_ramp[terrain.ci(i + 1, j)]
			&& terrain.corner_ramp[terrain.ci(i + 2, j)] && terrain.corner_ramp[terrain.ci(i, j - 1)]
			&& terrain.corner_ramp[terrain.ci(i, j - 2)]) {
			terrain.corner_ramp[terrain.ci(i + horizontal, j - 1)] = true;
		}
	}

	if (allow_ramp_vertical || allow_ramp_diagonal) {
		if (valid(i + 1, j + vertical) && terrain.corner_ramp[terrain.ci(i, j)] && terrain.corner_ramp[terrain.ci(i, j + 1)]
			&& terrain.corner_ramp[terrain.ci(i, j + 2)] && terrain.corner_ramp[terrain.ci(i + 1, j)]
			&& terrain.corner_ramp[terrain.ci(i + 2, j)]) {
			terrain.corner_ramp[terrain.ci(i + 1, j + vertical)] = true;
		}

		if (valid(i - 1, j + vertical) && terrain.corner_ramp[terrain.ci(i, j)] && terrain.corner_ramp[terrain.ci(i, j + 1)]
			&& terrain.corner_ramp[terrain.ci(i, j + 2)] && terrain.corner_ramp[terrain.ci(i - 1, j)]
			&& terrain.corner_ramp[terrain.ci(i - 2, j)]) {
			terrain.corner_ramp[terrain.ci(i - 1, j + vertical)] = true;
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
	auto& terrain = map->terrain;
	const size_t center_idx = terrain.ci(center_x, center_y);

	if (water_above_ground(center_idx)) {
		water_height = terrain.corner_water_height[center_idx];
	} else {
		int layer_height = terrain.corner_layer_height[center_idx];
		float terrain_height = layer_height - 2 + terrain.corner_height[center_idx];
		water_height = terrain_height + CellOperator::WATER_GROUND_ZERO + CellOperator::WATER_HEIGHT;
	}
}

QRect CellOperator::apply(const QRect& area, double frame_delta) {
	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;
	const glm::ivec2 pos = brush->get_unclipped_pos();

	bool edits_water = cell_operation_type == cell_operation::remove_water || cell_operation_type == cell_operation::add_water;

	// note: cell operator brush targets cells, not corners
	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			if (!brush->contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(pos, 0))) {
				continue;
			}

			// bounds check
			if (i >= width || j >= width || i < 0 || j < 0) {
				continue;
			}

			const size_t id = terrain.ci(i, j);

			// (i, j) is the bottom-left corner here
			if (cell_operation_type == cell_operation::add_water) {
				if (!terrain.corner_water[id]) {
					terrain.corner_water[id] = true;
					terrain.corner_water_height[id] = water_height;
				} else {
					// if the water is not visible (below the ground), we want to incerase it's height
					if (!water_above_ground(id)) {
						terrain.corner_water_height[id] = water_height;
					}
				}
			} else if (cell_operation_type == cell_operation::remove_water) {
				terrain.corner_water[id] = false;
				terrain.corner_water_height[id] = 0;
			} else if (cell_operation_type == cell_operation::add_boundary) {
				terrain.corner_boundary[id] = true;
			} else if (cell_operation_type == cell_operation::remove_boundary) {
				terrain.corner_boundary[id] = false;
			} else if (cell_operation_type == cell_operation::add_hole) {
				// future work
			} else if (cell_operation_type == cell_operation::remove_hole) {
				// future work
			}
		}
	}

	// finally, re-render the water if we changed it
	if (edits_water) {
		terrain.update_water(area.adjusted(0, 0, 1, 1).intersected({0, 0, width, height}));
	}

	// modified area (in pathing resolution)
	QRect modified_area;
	if (brush->brush_type == Brush::Type::corner) {
		modified_area = TerrainBrush::to_pathing_rect(area).adjusted(-2, -2, -2, -2);
	} else {
		modified_area = TerrainBrush::to_pathing_rect(area);
	}
	return modified_area;
}

void CellOperator::apply_end(WorldEditContext& ctx, const QRect& area) {
	QRect area_terrain = TerrainBrush::from_pathing_rect(area);

	if (cell_operation_type == cell_operation::remove_water || cell_operation_type == cell_operation::add_water) {
		brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::water);
	} else if (cell_operation_type == cell_operation::add_boundary || cell_operation_type == cell_operation::remove_boundary) {
		brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::texture);
	} else if (cell_operation_type == cell_operation::add_hole || cell_operation_type == cell_operation::remove_hole) {
		brush->add_terrain_undo(ctx, area_terrain, TerrainUndoType::cliff);
	}
}

bool CellOperator::water_above_ground(int corner_id) const {
	auto& terrain = map->terrain;
	return terrain.corner_water_height[corner_id]
		> terrain.corner_layer_height[corner_id] - 2 + terrain.corner_height[corner_id] + CellOperator::WATER_GROUND_ZERO;
}

void CellOperator::set_operation_type(cell_operation operation) {
	cell_operation_type = operation;
	if (operation == cell_operation::add_boundary || operation == cell_operation::remove_boundary) {
		set_brush_type(Brush::Type::cell);
	} else {
		set_brush_type(Brush::Type::corner);
	}
}

CellOperator::cell_operation CellOperator::get_operation_type() {
	return cell_operation_type;
}
