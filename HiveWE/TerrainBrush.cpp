#include "stdafx.h"

void TerrainBrush::apply() {
	const int x = position.x + brush_offset.x + 1;
	const int y = position.y + brush_offset.y + 1;
	const int cells = size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, map.terrain.width + 1, map.terrain.height + 1 });
	QRect area2 = QRect(x - 1, y - 1, cells + 1, cells + 1).intersected({ 0, 0, map.terrain.width, map.terrain.height});

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	const int id = map.terrain.ground_texture_to_id[tile_id];

	for (int i = area.x(); i < area.x() + area.width(); i++) {
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			map.terrain.corners[i][j].ground_texture = id;
			map.terrain.corners[i][j].ground_variation = get_random_variation();
		}
	}

	for (int i = area2.x(); i < area2.x() + area2.width(); i++) {
		for (int j = area2.y(); j < area2.y() + area2.height(); j++) {
			map.terrain.ground_texture_list[j * map.terrain.width + i] = map.terrain.get_texture_variations(i, j);
		}
	}

	const int offset = area2.y() * map.terrain.width + area2.x();
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, map.terrain.width);
	gl->glTextureSubImage2D(map.terrain.ground_texture_data, 0, area2.x(), area2.y(), area2.width(), area2.height(), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, map.terrain.ground_texture_list.data() + offset);
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

int TerrainBrush::get_random_variation() const {
	std::random_device rd;
	std::mt19937 e2(rd());
	const std::uniform_int_distribution<> dist(0, 570);

	int nr = dist(e2) - 1;

	const int id = map.terrain.ground_texture_to_id[tile_id];

	// If not extended
	if (map.terrain.ground_textures[id]->width != map.terrain.ground_textures[id]->height * 2) {
		return nr < 285 ? 0 : 15;
	}

	for (auto&&[variation, chance] : variation_chances) {
		if (nr < chance) {
			return variation;
		}
		nr -= chance;
	}
	static_assert("Should never come here");
	return 0;
}
