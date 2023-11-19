#include "tile_pather.h"

#include "ankerl/unordered_dense.h"

//#include "Globals.h"
#include <map_global.h>

#include <glad/glad.h>

import OpenGLUtilities;
import Texture;

TilePather::TilePather(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	ui.flowlayout_placeholder->addLayout(selected_layout);

	for (auto&&[tile_id, options] : map->terrain.pathing_options) {
		pathing_options[tile_id].unwalkable = options.unwalkable;
		pathing_options[tile_id].unflyable = options.unflyable;
		pathing_options[tile_id].unbuildable = options.unbuildable;
	}

	slk::SLK& slk = map->terrain.terrain_slk;
	for (auto&& i : map->terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i));
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({ 64, 64 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		selected_layout->addWidget(button);
		selected_group->addButton(button);
	}

	selected_group->buttons().first()->setChecked(true);
	changed_tile(selected_group->buttons().first());

	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TilePather::changed_tile);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePather::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(ui.unwalkable, &QPushButton::clicked, [&](bool checked) { pathing_options[current_tile].unwalkable = checked; });
	connect(ui.unflyable, &QPushButton::clicked, [&](bool checked) { pathing_options[current_tile].unflyable = checked; });
	connect(ui.unbuildable, &QPushButton::clicked, [&](bool checked) { pathing_options[current_tile].unbuildable = checked; });

	connect(ui.replaceType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::replace; });
	connect(ui.addType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::add; });
	connect(ui.removeType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::remove; });

	connect(ui.applyRetroactively, &QCheckBox::clicked, [&](bool checked) { pathing_options[current_tile].apply_retroactively = checked; });
}

void TilePather::changed_tile(QAbstractButton* button) {
	ui.selectedTileLabel->setText("Tile: " + button->property("tileName").toString());

	current_tile = button->property("tileID").toString().toStdString();

	ui.unwalkable->setChecked(pathing_options[current_tile].unwalkable);
	ui.unflyable->setChecked(pathing_options[current_tile].unflyable);
	ui.unbuildable->setChecked(pathing_options[current_tile].unbuildable);

	ui.applyRetroactively->setChecked(pathing_options[current_tile].apply_retroactively);

	switch (pathing_options[current_tile].operation) {
	case PathingOptions::Operation::replace:
		ui.replaceType->setChecked(true);
		break;
	case PathingOptions::Operation::add:
		ui.addType->setChecked(true);
		break;
	case PathingOptions::Operation::remove:
		ui.removeType->setChecked(true);
		break;
	}
}

void TilePather::save_tiles() {
	for (auto&&[tile_id, options] : pathing_options) {
		// Save state
		map->terrain.pathing_options[tile_id].unwalkable = options.unwalkable;
		map->terrain.pathing_options[tile_id].unflyable = options.unflyable;
		map->terrain.pathing_options[tile_id].unbuildable = options.unbuildable;

		if (!options.apply_retroactively) {
			continue;
		}

		uint8_t mask = 0;
		if (options.unwalkable) {
			mask |= 0b00000010;
		}
		if (options.unflyable) {
			mask |= 0b00000100;
		}
		if (options.unbuildable) {
			mask |= 0b00001000;
		}

		const int id = map->terrain.ground_texture_to_id.at(tile_id);
		for (int i = 0; i < map->terrain.width; i++) {
			for (int j = 0; j < map->terrain.height; j++) {
				if (map->terrain.real_tile_texture(i, j) != id) {
					continue;
				}

				const int left = std::max(i * 4 - 2, 0);
				const int bottom = std::max(j * 4 - 2, 0);

				const int right = std::min(i * 4 + 2, map->pathing_map.width);
				const int top = std::min(j * 4 + 2, map->pathing_map.height);

				for (int x = left; x < right; x++) {
					for (int y = bottom; y < top; y++) {
						uint8_t byte_cell = map->pathing_map.pathing_cells_static[y * map->pathing_map.width + x];

						switch (options.operation) {
						case PathingOptions::Operation::replace:
							byte_cell &= ~0b00001110;
							byte_cell |= mask;
							break;
						case PathingOptions::Operation::add:
							byte_cell |= mask;
							break;
						case PathingOptions::Operation::remove:
							byte_cell &= ~mask;
							break;
						}

						map->pathing_map.pathing_cells_static[y * map->pathing_map.width + x] = byte_cell;
					}
				}
			}
		}
	}

	glTextureSubImage2D(map->pathing_map.texture_static, 0, 0, 0, map->pathing_map.width, map->pathing_map.height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, map->pathing_map.pathing_cells_static.data());

	close();
}