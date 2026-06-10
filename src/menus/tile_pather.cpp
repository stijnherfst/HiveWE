#include "tile_pather.h"

import std;
import OpenGLUtilities;
import Texture;
import MapGlobal;
import PathingMap;
import Tileset;
import ResourceManager;
import <glad/glad.h>;

TilePather::TilePather(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	ui.flowlayout_placeholder->addLayout(selected_layout);

	for (const auto& i : map->terrain.tileset_ids) {
		const uint8_t pathing = map->tilesets.terrain_texture(i)->get_tile_pathing();
		pathing_options[i].unwalkable = pathing & PathingMap::Flags::unwalkable;
		pathing_options[i].unflyable = pathing & PathingMap::Flags::unflyable;
		pathing_options[i].unbuildable = pathing & PathingMap::Flags::unbuildable;
	}

	for (const auto& i : map->terrain.tileset_ids) {
		const auto& texture = *map->tilesets.terrain_texture(i);
		const auto image = resource_manager.load<Texture>(texture.file_path).value();
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({64, 64});
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromUtf8(texture.name));

		selected_layout->addWidget(button);
		selected_group->addButton(button);
	}

	selected_group->buttons().first()->setChecked(true);
	changed_tile(selected_group->buttons().first());

	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TilePather::changed_tile);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePather::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(ui.unwalkable, &QPushButton::clicked, [&](bool checked) {
		pathing_options[current_tile].unwalkable = checked;
	});
	connect(ui.unflyable, &QPushButton::clicked, [&](bool checked) {
		pathing_options[current_tile].unflyable = checked;
	});
	connect(ui.unbuildable, &QPushButton::clicked, [&](bool checked) {
		pathing_options[current_tile].unbuildable = checked;
	});

	connect(ui.replaceType, &QPushButton::clicked, [&]() {
		pathing_options[current_tile].operation = PathingOptions::Operation::replace;
	});
	connect(ui.addType, &QPushButton::clicked, [&]() {
		pathing_options[current_tile].operation = PathingOptions::Operation::add;
	});
	connect(ui.removeType, &QPushButton::clicked, [&]() {
		pathing_options[current_tile].operation = PathingOptions::Operation::remove;
	});

	connect(ui.applyRetroactively, &QCheckBox::clicked, [&](bool checked) {
		pathing_options[current_tile].apply_retroactively = checked;
	});
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
	for (const auto& [tile_id, options] : pathing_options) {
		// Save override pathing back to tilesets and "reset it" if it is the same as base-game pathing
		const uint8_t mask = (options.unwalkable ? PathingMap::Flags::unwalkable : 0)
			| (options.unflyable ? PathingMap::Flags::unflyable : 0) | (options.unbuildable ? PathingMap::Flags::unbuildable : 0);
		TerrainTexture* texture = map->tilesets.terrain_texture(tile_id);
		texture->override_pathing = (mask != texture->base_pathing) ? std::optional<uint8_t>(mask) : std::nullopt;

		if (!options.apply_retroactively) {
			continue;
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
								byte_cell &=
									~(PathingMap::Flags::unwalkable | PathingMap::Flags::unflyable | PathingMap::Flags::unbuildable);
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

	glTextureSubImage2D(
		map->pathing_map.texture_static,
		0,
		0,
		0,
		map->pathing_map.width,
		map->pathing_map.height,
		GL_RED_INTEGER,
		GL_UNSIGNED_BYTE,
		map->pathing_map.pathing_cells_static.data()
	);

	close();
}
