#include "stdafx.h"

bool PathingMap::load(BinaryReader& reader) {
	const std::string magic_number = reader.read_string(4);
	if (magic_number != "MP3W") {
		std::cout << "Invalid war3map.wpm file: Magic number is not MP3W" << std::endl;
		return false;
	}

	const int version = reader.read<uint32_t>();
	if (version != 0) {
		std::cout << "Unknown Pathmap version. Attempting to load, but may crash.";
	}

	width = reader.read<uint32_t>();
	height = reader.read<uint32_t>();

	pathing_cells_static = reader.read_vector<uint8_t>(width * height);
	pathing_cells_dynamic.resize(width * height);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &texture_static);
	gl->glTextureStorage2D(texture_static, 1, GL_R8UI, width, height);
	gl->glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
	gl->glTextureParameteri(texture_static, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture_static, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture_static, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(texture_static, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &texture_dynamic);
	gl->glTextureStorage2D(texture_dynamic, 1, GL_R8UI, width, height);
	const uint8_t clear_color = 0;
	gl->glClearTexImage(texture_dynamic, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_color);
	gl->glTextureParameteri(texture_dynamic, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture_dynamic, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;
}

void PathingMap::save() const {
	BinaryWriter writer;
	writer.write_string("MP3W");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(width);
	writer.write<uint32_t>(height);
	writer.write_vector<uint8_t>(pathing_cells_static);

	hierarchy.map_file_write("war3map.wpm", writer.buffer);
}

/// Clears an area with zeroes
void PathingMap::dynamic_clear_area(const QRect& area) {
	const QRect t = QRect(area.left() * 4, area.top() * 4, area.width() * 4, area.height() * 4).intersected({ 0, 0, width, height });

	for (int j = t.top(); j < t.bottom(); j++) {
		for (int i = t.left(); i < t.right(); i++) {
			pathing_cells_dynamic[j * width + i] = 0;
		}
	}
}

/// Blits a pathing texture to the specified location on the pathing map. Manually call update_dynamic() afterwards to upload the changes to the GPU
/// Expects position in whole grid tiles and draws the texture centered around this position
/// Rotation in multiples of 90
/// Blits the texture upside down as OpenGL uses the bottom-left as 0,0
void PathingMap::blit_pathing_texture(glm::vec2 position, int rotation, const std::shared_ptr<Texture>& pathing_texture) {
	for (int j = 0; j < pathing_texture->height; j++) {
		for (int i = 0; i < pathing_texture->width; i++) {
			int x = i;
			int y = j;

			switch (rotation) {
				case 90:
					x = pathing_texture->width - 1 - j - std::max(0, pathing_texture->width - pathing_texture->height);
					y = i + std::max(0, pathing_texture->height - pathing_texture->width);
					break;
				case 180:
					x = pathing_texture->width - 1 - i;
					y = pathing_texture->height - 1 - j;
					break;
				case 270:
					x = j + std::max(0, pathing_texture->height - pathing_texture->width);
					y = pathing_texture->height - 1 - i + std::max(0, pathing_texture->width - pathing_texture->height);
					break;
			}

			// Width and height for centering change if rotation is not divisible by 180
			const int div_w = (rotation % 180) ? pathing_texture->height : pathing_texture->width;
			const int div_h = (rotation % 180) ? pathing_texture->width : pathing_texture->height;
			const int xx = position.x * 4 + x - div_w / 2;
			const int yy = position.y * 4 + y - div_h / 2;

			if (xx < 0 || xx > width - 1 || yy < 0 || yy > height - 1) {
				continue;
			}

			const unsigned int index = ((pathing_texture->height - 1 - j) * pathing_texture->width + i) * pathing_texture->channels;

			uint8_t bytes = (pathing_texture->data[index] > 250) * Flags::unwalkable
				| (pathing_texture->data[index + 1] > 250) * Flags::unflyable
				| (pathing_texture->data[index + 2] > 250) * Flags::unbuildable;

			pathing_cells_dynamic[yy * width + xx] |= bytes;
		}
	}
}

void PathingMap::upload_static_pathing() {
	gl->glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
}

void PathingMap::upload_dynamic_pathing() {
	gl->glTextureSubImage2D(texture_dynamic, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_dynamic.data());
}


void PathingMap::new_undo_group() {
	old_pathing_cells_static = pathing_cells_static;
}

void PathingMap::add_undo(const QRect& area) {
	auto undo_action = std::make_unique<PathingMapAction>();

	undo_action->area = area;

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
			undo_action->new_pathing.push_back(pathing_cells_static[j * width + i]);
		}
	}

	map->terrain_undo.add_undo_action(std::move(undo_action));
}

void PathingMapAction::undo() {
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			map->pathing_map.pathing_cells_static[j * map->pathing_map.width + i] = old_pathing[(j - area.top()) * area.width() + i - area.left()];
		}
	}
	map->pathing_map.upload_static_pathing();
}

void PathingMapAction::redo() {
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			map->pathing_map.pathing_cells_static[j * map->pathing_map.width + i] = new_pathing[(j - area.top()) * area.width() + i - area.left()];
		}
	}
	map->pathing_map.upload_static_pathing();
}