#include "pathing_brush.h"

#include <QRect>

import <glm/glm.hpp>;
import MapGlobal;
import PathingUndo;

PathingBrush::PathingBrush() : Brush() {
	brush_offset = {0.125f, 0.125f};
	granularity = 4.f;
}

void PathingBrush::apply_begin() {
	const int x = position.x * 4 + uv_offset.x;
	const int y = position.y * 4 + uv_offset.y;

	applied_area = QRect(x, y, size, size).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });

	map->world_undo.new_undo_group();
	old_pathing_cells_static = map->pathing_map.pathing_cells_static;
}

void PathingBrush::apply(double frame_delta) {
	const int x = position.x * 4 + uv_offset.x;
	const int y = position.y * 4 + uv_offset.y;

	QRect area = QRect(x, y, size, size).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });
	
	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	const int offset = area.y() * map->pathing_map.width + area.x();

	for (int i = 0; i < area.width(); i++) {
		for (int j = 0; j < area.height(); j++) {
			if (!contains(i - std::min(x, 0), j - std::min(y, 0))) {
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