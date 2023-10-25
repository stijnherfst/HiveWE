#include "pathing_brush.h"

#include <QRect>

//#include "Globals.h"
#include <map_global.h>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

PathingBrush::PathingBrush() : Brush() {
	brush_offset = {0.125f, 0.125f};
	granularity = 4.f;
}

void PathingBrush::apply_begin() {
	const int x = position.x * 4 + uv_offset.x;
	const int y = position.y * 4 + uv_offset.y;

	applied_area = QRect(x, y, size, size).intersected({ 0, 0, map->pathing_map.width, map->pathing_map.height });

	map->terrain_undo.new_undo_group();
	map->pathing_map.new_undo_group();
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
	map->terrain_undo.add_undo_action(map->pathing_map.add_undo(applied_area));

	//map->pathing_map.add_undo(applied_area);
}