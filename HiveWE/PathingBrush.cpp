#include "stdafx.h"

void PathingBrush::apply() {
	const int x = position.x * 4 + uv_offset.x;
	const int y = position.y * 4 + uv_offset.y;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, int(map.pathing_map.width), int(map.pathing_map.height) });

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	const int offset = area.y() * map.pathing_map.width + area.x();

	for (int i = 0; i < area.width(); i++) {
		for (int j = 0; j < area.height(); j++) {
			const int index = offset + j * map.pathing_map.width + i;
			switch (operation) {
			case Operation::replace:
				map.pathing_map.pathing_cells[index] &= ~0b00001110;
				map.pathing_map.pathing_cells[index] |= brush_mask;
				break;
			case Operation::add:
				map.pathing_map.pathing_cells[index] |= brush_mask;
				break;
			case Operation::remove:
				map.pathing_map.pathing_cells[index] &= ~brush_mask;
				break;
			}
		}
	}

	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.pathing_map.width);
	gl->glTextureSubImage2D(map.pathing_map.pathing_texture, 0, area.x(), area.y(), area.width(), area.height(), GL_RED_INTEGER, GL_UNSIGNED_BYTE, map.pathing_map.pathing_cells.data() + offset);
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}