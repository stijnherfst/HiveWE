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
	uint8_t clear_color = 0;
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

	hierarchy.map.file_write("war3map.wpm", writer.buffer);
}

void PathingMap::update_dynamic() {
	for (const auto& i : map->doodads.doodads) {
		if (!i.pathing || doodads_slk.row_header_exists(i.id)) {
			continue;
		}

		for (int j = 0; j < i.pathing->height; j++) {
			for (int k = 0; k < i.pathing->width; k++) {
				int x = i.position.x * 4 + k - i.pathing->width / 2;
				int y = i.position.y * 4 + j - i.pathing->height / 2;

				if (x < 0 || x > width || y < 0 || y > height) {
					continue;
				}

				unsigned int index = (j * i.pathing->height + k) * i.pathing->channels;

				uint8_t bytes = i.pathing->data[index] & Flags::unwalkable
					| i.pathing->data[index + 1] & Flags::unflyable
					| i.pathing->data[index + 2] & Flags::unbuildable;

				pathing_cells_dynamic[y * width + x] |= bytes;
			}
		}
	}

	gl->glTextureSubImage2D(texture_dynamic, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_dynamic.data());
}

void PathingMap::upload_static_pathing() {
	gl->glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
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