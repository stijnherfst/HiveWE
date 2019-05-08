#include "stdafx.h"

PathingBrush::PathingBrush() {
	brush_offset = {0.125f, 0.125f};
}

void PathingBrush::apply_begin() {
	const int x = position.x * 4 + uv_offset.x;
	const int y = position.y * 4 + uv_offset.y;

	applied_area = QRect(x, y, size, size).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });

	map->terrain_undo.new_undo_group();
	map->pathing_map.new_undo_group();
}

void PathingBrush::apply() {
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
	map->pathing_map.add_undo(applied_area);
}