#include "pathing_brush.h"

#include <QRect>

import std;
import <glm/glm.hpp>;
import MapGlobal;
import PathingUndo;
import Camera;

PathingBrush::PathingBrush() : Brush() {
	position_granularity = 4.f;
	size_granularity = 1;
	center_on_tile_corner = false;
}

void PathingBrush::apply_begin() {
	const glm::ivec2 pos = glm::vec2(input_handler.mouse_world) * 4.f - size.x / 2.f + 0.5f;
	const int x = pos.x;
	const int y = pos.y;

	applied_area = QRect(x, y, size.x, size.y).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });

	map->world_undo.new_undo_group();
	old_pathing_cells_static = map->pathing_map.pathing_cells_static;
}

void PathingBrush::apply(double frame_delta) {
	const glm::ivec2 pos = glm::vec2(input_handler.mouse_world) * 4.f - size.x / 2.f + 0.5f;
	const QRect area = QRect(pos.x, pos.y, size.x, size.y).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });
	
	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	const int offset = area.y() * map->pathing_map.width + area.x();

	for (int i = 0; i < area.width(); i++) {
		for (int j = 0; j < area.height(); j++) {
			if (!contains(glm::ivec2(i - std::min(pos.x, 0), j - std::min(pos.y, 0)))) {
				continue;
			}

			const int index = offset + j * map->pathing_map.width + i;
			switch (operation) {
				case Operation::replace:
					map->pathing_map.pathing_cells_static[index] &= ~0b00001110;
					map->pathing_map.pathing_cells_static[index] |= brush_mask;
					break;
				case Operation::add:
					map->pathing_map.pathing_cells_static[index] |= brush_mask;
					break;
				case Operation::remove:
					map->pathing_map.pathing_cells_static[index] &= ~brush_mask;
					break;
			}
		}
	}

	applied_area = applied_area.united(area);

	map->pathing_map.upload_static_pathing();
}

void PathingBrush::apply_end() {
	add_pathing_undo(applied_area);
}

void PathingBrush::add_pathing_undo(const QRect& area) {
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