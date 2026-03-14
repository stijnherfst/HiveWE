#include "terrain_brush.h"

import std;
import MapGlobal;
import Terrain;
import DoodadsUndo;
import PathingUndo;
import TerrainUndo;
import Camera;

TerrainBrush::TerrainBrush() : Brush() {
	position_granularity = 1.f;
	size_granularity = 4;
	center_on_tile_corner = true;

	set_size(size);
}

<<<<<<< HEAD
=======
void TerrainBrush::setup_operators() {
	height_operator = new HeightOperator(this);
	texture_operator = new TextureOperator(this);
	cliff_operator = new CliffOperator(this);
	cell_operator = new CellOperator(this);

	terrain_operators = {height_operator, texture_operator, cliff_operator, cell_operator};
}

void TerrainBrush::deactivate_operator(TerrainOperator* target) {
	if (target) {
		target->is_active = false;
	}
}

void TerrainBrush::activate_operator(TerrainOperator* target) {
	if (target) {
		target->is_active = true;
		center_on_tile_corner = target->center_on_tile_corner;

		// deactivate incompatible operators
		for (auto* op : terrain_operators) {
			if (op != target && !target->can_combine_with(op)) {
				op->is_active = false;
			}
		}
	}
}

>>>>>>> dc0dadb (new terrain operations)
void TerrainBrush::mouse_press_event(QMouseEvent* event, double frame_delta) {
	//if (event->button() == Qt::LeftButton && mode == Mode::selection && !event->modifiers() && input_handler.mouse.y > 0.f) {
	/*auto id = map->render_manager.pick_unit_id_under_mouse(map->units, input_handler.mouse);
		if (id) {
			Unit& unit = map->units.units[id.value()];
			selections = { &unit };
			dragging = true;
			drag_x_offset = input_handler.mouse_world.x - unit.position.x;
			drag_y_offset = input_handler.mouse_world.y - unit.position.y;
			return;
		}*/
	//}

	Brush::mouse_press_event(event, frame_delta);
}

void TerrainBrush::mouse_move_event(QMouseEvent* event, double frame_delta) {
	Brush::mouse_move_event(event, frame_delta);

	/*if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
			if (dragging) {
				if (!dragged) {
					dragged = true;
					map->terrain_undo.new_undo_group();
					unit_state_undo = std::make_unique<UnitStateAction>();
					for (const auto& i : selections) {
						unit_state_undo->old_units.push_back(*i);
					}
				}
				for (auto& i : selections) {
					i->position.x = input_handler.mouse_world.x - drag_x_offset;
					i->position.y = input_handler.mouse_world.y - drag_y_offset;
					i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y);
					i->update();
				}
			} else if (event->modifiers() & Qt::ControlModifier) {
				for (auto&& i : selections) {
					float target_rotation = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					if (target_rotation < 0) {
						target_rotation = (glm::pi<float>() + target_rotation) + glm::pi<float>();
					}

					i->angle = target_rotation;
					i->update();
				}
			} else if (selection_started) {
				const glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
				selections = map->units.query_area({ selection_start.x, selection_start.y, size.x, size.y });
			}
		}
	}*/
}

void TerrainBrush::mouse_release_event(QMouseEvent* event) {
	//dragging = false;
	//if (dragged) {
	//	dragged = false;
	//	for (const auto& i : selections) {
	//		unit_state_undo->new_units.push_back(*i);
	//	}
	//	map->terrain_undo.add_undo_action(std::move(unit_state_undo));
	//}

	Brush::mouse_release_event(event);
}

// Make this an iterative function instead to avoid stack overflows
void TerrainBrush::check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const {
	const QRect bounds = QRect(i - 1, j - 1, 3, 3).intersected({0, 0, map->terrain.width, map->terrain.height});

	for (int k = bounds.x(); k <= bounds.right(); k++) {
		for (int l = bounds.y(); l <= bounds.bottom(); l++) {
			if (k == 0 && l == 0) {
				continue;
			}

			const int difference = map->terrain.corner_layer_height[map->terrain.ci(i, j)] - map->terrain.corner_layer_height[map->terrain.ci(k, l)];
			if (std::abs(difference) > 2 && !contains(glm::ivec2(begx + (k - i), begy + (l - k)))) {
				map->terrain.corner_layer_height[map->terrain.ci(k, l)] = map->terrain.corner_layer_height[map->terrain.ci(i, j)] - std::clamp(difference, -2, 2);
				map->terrain.corner_ramp[map->terrain.ci(k, l)] = false;

				area.setX(std::min(area.x(), k - 1));
				area.setY(std::min(area.y(), l - 1));
				area.setRight(std::max(area.right(), k));
				area.setBottom(std::max(area.bottom(), l));

				check_nearby(begx, begy, k, l, area);
			}
		}
	}
}

void TerrainBrush::apply_begin() {
	const int width = map->terrain.width;
	const int height = map->terrain.height;
	const auto& terrain = map->terrain;

	const glm::ivec2 pos = center_on_tile_corner ? glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 1.f
												 : glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 0.5f;
	QRect area = QRect(pos.x, pos.y, size.x / 4.f, size.y / 4.f).intersected({0, 0, width, height});

	const int center_x = area.x() + area.width() * 0.5f;
	const int center_y = area.y() + area.height() * 0.5f;

<<<<<<< HEAD
	// Setup for undo/redo — snapshot all corners
=======
	// setup for undo/redo
>>>>>>> dc0dadb (new terrain operations)
	map->world_undo.new_undo_group();
	old_corners_width = width;
	old_corners_height = height;
	old_corners.resize(width * height);
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			old_corners[j * width + i] = terrain.get_corner(i, j);
		}
	}
	old_pathing_cells_static = map->pathing_map.pathing_cells_static;
	texture_height_area = area;
	cliff_area = area;

<<<<<<< HEAD
	const size_t center_idx = terrain.ci(center_x, center_y);

	if (apply_height) {
		deformation_height = terrain.corner_height[center_idx];
	}

	if (apply_cliff) {
		layer_height = terrain.corner_layer_height[center_idx];
		switch (cliff_operation_type) {
			case cliff_operation::shallow_water:
				if (!terrain.corner_water[center_idx]) {
					layer_height -= 1;
				} else if (terrain.corner_final_water_height(center_x, center_y)
						   > terrain.corner_final_ground_height(center_x, center_y) + 1) {
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
				} else if (terrain.corner_final_water_height(center_x, center_y)
						   < terrain.corner_final_ground_height(center_x, center_y) + 1) {
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
=======
	// apply all active operators
	for (TerrainOperator* op : terrain_operators) {
		if (op->is_enabled()) {
			op->apply_begin(area, center_x, center_y);
>>>>>>> dc0dadb (new terrain operations)
		}
		layer_height = std::clamp(layer_height, Terrain::min_layer_height, Terrain::max_layer_height);
	}
}

void TerrainBrush::apply(double frame_delta) {
	int width = map->terrain.width;
	int height = map->terrain.height;
	auto& terrain = map->terrain;

	const glm::ivec2 pos = center_on_tile_corner ? glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 1.f
												 : glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 0.5f;

	QRect area = QRect(pos.x, pos.y, size.x / 4.f, size.y / 4.f).intersected({0, 0, width, height});
<<<<<<< HEAD
=======
	QRect updated_area = QRect(pos.x - 1, pos.y - 1, size.x / 4.f + 1, size.y / 4.f + 1).intersected({0, 0, width - 1, height - 1});
>>>>>>> dc0dadb (new terrain operations)

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	if (apply_texture) {
		const int id = terrain.ground_texture_to_id[tile_id];

		// Update textures
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				if (!contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
					continue;
				}

				const size_t idx = terrain.ci(i, j);
				if (id == terrain.blight_texture) {
					bool cliff_near = false;

					const int left = i > 0 ? -1 : 0;
					const int bottom = j > 0 ? -1 : 0;
					for (int k = left; k <= 0; k++) {
						for (int l = bottom; l <= 0; l++) {
							cliff_near = cliff_near || terrain.corner_cliff[terrain.ci(i + k, j + l)];
						}
					}

					// Blight shouldn't be set when there is a cliff near, following vanilla WE behavior.
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
		texture_height_area = texture_height_area.united(area);
	}

	if (apply_height) {
		std::vector<std::vector<float>> heights(area.width(), std::vector<float>(area.height()));

		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				const size_t idx = terrain.ci(i, j);
				float new_height = terrain.corner_height[idx];
				heights[i - area.x()][j - area.y()] = new_height;

				if (!contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
					continue;
				}
				const int center_x = area.x() + area.width() * 0.5f;
				const int center_y = area.y() + area.height() * 0.5f;

				switch (deformation_type) {
					case deformation::raise: {
						const auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						new_height += std::max(0.0, 1 - distance / size.x * std::sqrt(2)) * frame_delta;
						break;
					}
					case deformation::lower: {
						const auto distance = std::sqrt(std::pow(center_x - i, 2) + std::pow(center_y - j, 2));
						new_height -= std::max(0.0, 1 - distance / size.x * std::sqrt(2)) * frame_delta;
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
								if ((k < i || l < j) && k <= i &&
									// The checks below are required because the code is just wrong (area is too small so we cant convolute a cell from at the edges of the area)
									k - area.x() >= 0 && l - area.y() >= 0 && k < area.right() + 1 && l < area.bottom() + 1) {
									accumulate += heights[k - area.x()][l - area.y()];
								} else {
									accumulate += terrain.corner_height[terrain.ci(k, l)];
								}
							}
						}
						accumulate -= new_height;
						new_height = 0.8 * new_height + 0.2 * (accumulate / (acum_area.width() * acum_area.height() - 1));
						break;
					}
				}

				terrain.corner_height[idx] = std::clamp(new_height, Terrain::min_ground_height, Terrain::max_ground_height);
			}
		}

		terrain.update_ground_heights(area);

		texture_height_area = texture_height_area.united(area);
	}

	QRect updated_area = QRect(pos.x - 1, pos.y - 1, size.x / 4.f + 1, size.y / 4.f + 1).intersected({0, 0, width - 1, height - 1});

	if (apply_cliff) {
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
				if (!contains(glm::ivec2(i - area.x(), j - area.y()) - glm::min(glm::ivec2(position) + 1, 0))) {
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
							if (enforce_water_height_limits
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
					case cliff_operation::ramp:
						break;
				}

				check_nearby(pos.x, pos.y, i, j, updated_area);
			}
		}
		//}

		// Bounds check
		updated_area = updated_area.intersected({0, 0, width - 1, height - 1});

		// Determine if cliff
		for (int i = updated_area.x(); i <= updated_area.right(); i++) {
			for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
				const size_t bl = terrain.ci(i, j);
				const size_t br = terrain.ci(i + 1, j);
				const size_t tl = terrain.ci(i, j + 1);
				const size_t tr = terrain.ci(i + 1, j + 1);

				terrain.corner_cliff[bl] = terrain.corner_layer_height[bl] != terrain.corner_layer_height[br]
					|| terrain.corner_layer_height[bl] != terrain.corner_layer_height[tl] || terrain.corner_layer_height[bl] != terrain.corner_layer_height[tr];

				if (cliff_operation_type != cliff_operation::ramp) {
					terrain.corner_cliff_texture[bl] = cliff_id;
				}
			}
		}

		QRect tile_area = updated_area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});

		terrain.update_cliff_meshes(tile_area);
		terrain.update_ground_textures(updated_area);
		terrain.update_ground_heights(updated_area.adjusted(0, 0, 1, 1));
		terrain.update_water(tile_area.adjusted(0, 0, 1, 1));

		cliff_area = cliff_area.united(updated_area);

		if (cliff_operation_type == cliff_operation::shallow_water || cliff_operation_type == cliff_operation::deep_water) {
			terrain.upload_water_heights();
		}
	}

	// Apply pathing
	for (int i = updated_area.x(); i <= updated_area.right(); i++) {
		for (int j = updated_area.y(); j <= updated_area.bottom(); j++) {
<<<<<<< HEAD
			const size_t bl_idx = terrain.ci(i, j);

=======
>>>>>>> dc0dadb (new terrain operations)
			for (int k = 0; k < 4; k++) {
				for (int l = 0; l < 4; l++) {
					const size_t pathing_x = i * 4 + k;
					const size_t pathing_y = j * 4 + l;

<<<<<<< HEAD
					uint8_t mask = 0;
					if ((terrain.corner_cliff[bl_idx] || terrain.corner_romp[bl_idx]) && !terrain.is_corner_ramp_entrance(i, j) && apply_cliff_pathing) {
						mask = 0b00001010;
					}

					if (!terrain.corner_cliff[bl_idx] || (terrain.corner_ramp[bl_idx] && !terrain.corner_romp[bl_idx])) {
						const int cx = i + k / 2;
						const int cy = j + l / 2;
						const size_t cidx = terrain.ci(cx, cy);
						if (apply_tile_pathing) {
							const int id = terrain.corner_ground_texture[cidx];
							mask |= terrain.pathing_options[terrain.tileset_ids[id]].mask();
						}

						if (terrain.corner_water[cidx] && apply_water_pathing) {
							mask |= 0b01000000;
							if (terrain.corner_final_water_height(cx, cy) > terrain.corner_final_ground_height(cx, cy) + 0.40) {
								mask |= 0b00001010;
							} else if (terrain.corner_final_water_height(cx, cy) > terrain.corner_final_ground_height(cx, cy)) {
								mask |= 0b00001000;
							}
						}
					}
					map->pathing_map.pathing_cells_static[(j * 4 + l) * map->pathing_map.width + i * 4 + k] |= mask;
=======
					map->pathing_map.pathing_cells_static[pathing_y * map->pathing_map.width + pathing_x] &= ~0b01001110;

					uint8_t mask =
						map->terrain
							.get_terrain_pathing(pathing_x, pathing_y, apply_tile_pathing, apply_cliff_pathing, apply_water_pathing);

					map->pathing_map.pathing_cells_static[pathing_y * map->pathing_map.width + pathing_x] |= mask;
>>>>>>> dc0dadb (new terrain operations)
				}
			}
		}
	}

	map->pathing_map.upload_static_pathing();

	if (apply_height || apply_cliff) {
		if (change_doodad_heights) {
			for (auto&& i : map->doodads.doodads) {
				if (area.contains(i.position.x, i.position.y)) {
					if (std::find_if(
							pre_change_doodads.begin(),
							pre_change_doodads.end(),
							[i](const Doodad& doodad) {
								return doodad.creation_number == i.creation_number;
							}
						)
						== pre_change_doodads.end()) {
						pre_change_doodads.push_back(i);
					}
					i.position.z = terrain.interpolated_height(i.position.x, i.position.y, true);
					i.update(terrain);
					post_change_doodads[i.creation_number] = i;
				}
			}
		}
		map->units.update_area(updated_area, terrain);
	}
}

void TerrainBrush::apply_end() {
	if (apply_texture) {
		add_terrain_undo(texture_height_area, TerrainUndoType::texture);
	}

	if (apply_height) {
		add_terrain_undo(texture_height_area, TerrainUndoType::height);
	}

	if (apply_cliff) {
		QRect cliff_areaa = cliff_area.adjusted(0, 0, 1, 1).intersected({0, 0, map->terrain.width, map->terrain.height});
		add_terrain_undo(cliff_areaa, TerrainUndoType::cliff);
	}

	if (change_doodad_heights) {
		auto undo = std::make_unique<DoodadStateAction>();
		undo->old_doodads = pre_change_doodads;
		for (const auto& [id, doodad] : post_change_doodads) {
			undo->new_doodads.push_back(doodad);
		}
		pre_change_doodads.clear();
		post_change_doodads.clear();
		map->world_undo.add_undo_action(std::move(undo));
	}

	QRect pathing_area = QRect(cliff_area.x() * 4, cliff_area.y() * 4, cliff_area.width() * 4, cliff_area.height() * 4)
							 .adjusted(-2, -2, 2, 2)
							 .intersected({0, 0, map->pathing_map.width, map->pathing_map.height});
	add_pathing_undo(pathing_area);

	map->terrain.update_minimap();
}

/// Adds the undo to the current undo group
void TerrainBrush::add_terrain_undo(const QRect& area, TerrainUndoType type) {
	auto undo_action = std::make_unique<TerrainGenericAction>();

	undo_action->area = area;
	undo_action->undo_type = type;

	// Copy old corners
	undo_action->old_corners.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->old_corners.push_back(old_corners[j * old_corners_width + i]);
		}
	}

	// Copy new corners
	undo_action->new_corners.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->new_corners.push_back(map->terrain.get_corner(i, j));
		}
	}

	map->world_undo.add_undo_action(std::move(undo_action));
}

void TerrainBrush::add_pathing_undo(const QRect& area) {
	auto undo_action = std::make_unique<PathingMapAction>();

	undo_action->area = area;
	const auto width = map->pathing_map.width;

	// Copy old corners
	undo_action->old_pathing.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->old_pathing.push_back(old_pathing_cells_static[j * width + i]);
		}
	}

	// Copy new corners
	undo_action->new_pathing.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->new_pathing.push_back(map->pathing_map.pathing_cells_static[j * width + i]);
		}
	}

	map->world_undo.add_undo_action(std::move(undo_action));
}
