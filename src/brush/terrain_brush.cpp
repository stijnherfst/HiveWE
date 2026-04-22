#include "terrain_brush.h"
#include "terrain_operators.h"

import std;
import MapGlobal;
import Terrain;
import DoodadsUndo;
import PathingUndo;
import TerrainUndo;
import Camera;

TerrainBrush::TerrainBrush() :
	Brush(),
	cliff_operator(*this),
	height_operator(*this),
	texture_operator(*this),
	cell_operator(*this),
	terrain_operators({height_operator, texture_operator, cliff_operator, cell_operator}) {
	position_granularity = 1.f;
	size_granularity = 4;
	brush_type = Brush::Type::corner;

	set_size(size);
}

void TerrainBrush::deactivate_operator(TerrainOperator& target) {
	target.is_active = false;
}

void TerrainBrush::activate_operator(TerrainOperator& target) {
	target.is_active = true;
	brush_type = target.brush_type;

	// deactivate incompatible operators
	for (TerrainOperator& op : terrain_operators) {
		if (&op != &target && !can_combine(target, op)) {
			op.is_active = false;
		}
	}
}

bool TerrainBrush::can_combine(const TerrainOperator& a, const TerrainOperator& b) const {
	using TI = std::type_index;
	using Pair = std::pair<TI, TI>;

	// pairs of operator types that are allowed to be active simultaneously
	static const std::array compatible = {
		Pair {typeid(TextureOperator), typeid(CliffOperator)},
	};

	return std::ranges::any_of(compatible, [ta = TI(typeid(a)), tb = TI(typeid(b))](const Pair& p) {
		return (p.first == ta && p.second == tb) || (p.first == tb && p.second == ta);
	});
}

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

bool TerrainBrush::has_active_operators() {
	for (TerrainOperator& op : terrain_operators) {
		if (op.is_enabled()) {
			return true;
		}
	}

	return false;
}

void TerrainBrush::apply_begin() {
	// do nothing if there are no active operators
	if (!has_active_operators()) {
		return;
	}

	const auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;

	const glm::ivec2 pos = get_unclipped_pos();
	QRect area = QRect(pos.x, pos.y, size.x / 4.f, size.y / 4.f).intersected({0, 0, width, height});
	updated_area = QRect();

	const int center_x = area.x() + area.width() * 0.5f;
	const int center_y = area.y() + area.height() * 0.5f;

	// setup for undo/redo — snapshot all corners
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

	// apply all active operators
	for (TerrainOperator& op : terrain_operators) {
		if (op.is_enabled()) {
			op.apply_begin(area, center_x, center_y);
		}
	}
}

void TerrainBrush::apply(double frame_delta) {
	// do nothing if there are no active operators
	if (!has_active_operators()) {
		return;
	}

	auto& terrain = map->terrain;
	const int width = terrain.width;
	const int height = terrain.height;

	const glm::ivec2 pos = get_unclipped_pos();

	QRect area = QRect(pos.x, pos.y, size.x / 4.f, size.y / 4.f).intersected({0, 0, width, height});
	QRect affected_area = QRect();

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	// apply all active operators
	for (TerrainOperator& op : terrain_operators) {
		if (op.is_active) {
			affected_area =
				affected_area.united(op.apply(area, frame_delta)).intersected({0, 0, map->pathing_map.width, map->pathing_map.height});
		}
	}

	// entire area modified so far
	updated_area = updated_area.united(affected_area);

	// apply pathing
	for (size_t i = affected_area.x(); i <= affected_area.right(); i++) {
		for (size_t j = affected_area.y(); j <= affected_area.bottom(); j++) {
			map->pathing_map.pathing_cells_static[j * map->pathing_map.width + i] &= ~0b01001110;
			uint8_t mask = map->terrain.get_terrain_pathing(i, j, apply_tile_pathing, apply_cliff_pathing, apply_water_pathing);
			map->pathing_map.pathing_cells_static[j * map->pathing_map.width + i] |= mask;
		}
	}

	map->pathing_map.upload_static_pathing();

	if (height_operator.is_active || cliff_operator.is_active) {
		QRectF object_area =
			QRectF(updated_area.x() / 4.f, updated_area.y() / 4.f, updated_area.width() / 4.f, updated_area.height() / 4.f);

		if (change_doodad_heights) {
			for (auto&& i : map->doodads.doodads) {
				if (object_area.contains(i.position.x, i.position.y)) {
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
					i.position.z = map->terrain.interpolated_height(i.position.x, i.position.y, true);
					i.update(map->terrain);
					post_change_doodads[i.creation_number] = i;
				}
			}
		}

		map->units.update_area(object_area, map->terrain);
	}
}

void TerrainBrush::apply_end() {
	// do nothing if there are no active operators
	if (!has_active_operators()) {
		return;
	}

	WorldEditContext ctx {
		.terrain = map->terrain,
		.units = map->units,
		.doodads = map->doodads,
		.brush = this,
		.pathing_map = map->pathing_map,
	};

	// apply all active operators
	for (TerrainOperator& op : terrain_operators) {
		if (op.is_active) {
			op.apply_end(ctx, updated_area);
		}
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

	add_pathing_undo(ctx, updated_area);

	map->terrain.update_minimap();
}

/// Adds the undo to the current undo group
void TerrainBrush::add_terrain_undo(WorldEditContext& ctx, const QRect& area, TerrainUndoType type) {
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
			undo_action->new_corners.push_back(ctx.terrain.get_corner(i, j));
		}
	}

	map->world_undo.add_undo_action(std::move(undo_action));
}

void TerrainBrush::add_pathing_undo(WorldEditContext& ctx, const QRect& area) {
	auto undo_action = std::make_unique<PathingMapAction>();

	undo_action->area = area;
	const auto width = ctx.pathing_map.width;

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
			undo_action->new_pathing.push_back(ctx.pathing_map.pathing_cells_static[j * width + i]);
		}
	}

	map->world_undo.add_undo_action(std::move(undo_action));
}

glm::ivec2 TerrainBrush::get_unclipped_pos() const {
	const glm::vec2 fpos = brush_type == Brush::Type::corner ? glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 1.f
															 : glm::vec2(input_handler.mouse_world) - size.x / 4.f / 2.f + 0.5f;
	return glm::ivec2(glm::floor(fpos));
}

/// Converts a rect in pathing resolution to a rect in terrain corner resolution
QRect TerrainBrush::from_pathing_rect(const QRect& rect) {
	int x = rect.x() / 4;
	int y = rect.y() / 4;
	int right = (rect.x() + rect.width() + 3) / 4;
	int bottom = (rect.y() + rect.height() + 3) / 4;
	int width = right - x + 1;
	int height = bottom - y + 1;
	QRect result(x, y, width, height);
	return result;
}

/// Converts a rect in terrain resolution to a rect in pathing resolution
QRect TerrainBrush::to_pathing_rect(const QRect& rect) {
	return QRect(rect.x() * 4, rect.y() * 4, rect.width() * 4, rect.height() * 4);
}
