#include "tile_pather.h"

import std;
import OpenGLUtilities;
import Texture;
import MapGlobal;
import PathingMap;
import Tileset;
import ResourceManager;
import Terrain;
import <glad/glad.h>;

TilePather::TilePather(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	ui.flowlayout_placeholder->addLayout(selected_layout);

	for (const auto& i : map->terrain.tileset_ids) {
		const uint8_t pathing = map->tilesets.terrain_texture(i)->get_tile_pathing();
		pathing_options[i].unwalkable = pathing & PathingMap::Flags::unwalkable;
		pathing_options[i].unflyable = pathing & PathingMap::Flags::unflyable;
		pathing_options[i].unbuildable = pathing & PathingMap::Flags::unbuildable;
	}

	for (const auto& i : map->terrain.tileset_ids) {
		const TerrainTexture* texture = map->tilesets.terrain_texture(i);
		if (texture) {
			TextureButton* button = create_tex_button(texture);
			selected_layout->addWidget(button);
			selected_group->addButton(button);
		}
	}

	selected_group->buttons().first()->setChecked(true);
	changed_tile(selected_group->buttons().first());

	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TilePather::changed_tile);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePather::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(ui.unwalkable, &QPushButton::clicked, [&](const bool checked) {
		pathing_options[selected_button->texture()->id].unwalkable = checked;
		update_selected_icon();
	});
	connect(ui.unflyable, &QPushButton::clicked, [&](const bool checked) {
		pathing_options[selected_button->texture()->id].unflyable = checked;
		update_selected_icon();
	});
	connect(ui.unbuildable, &QPushButton::clicked, [&](const bool checked) {
		pathing_options[selected_button->texture()->id].unbuildable = checked;
		update_selected_icon();
	});

	show();
}

uint8_t TilePather::get_mask(const PathingOptions& options) const {
	return (options.unwalkable ? PathingMap::Flags::unwalkable : 0) | (options.unflyable ? PathingMap::Flags::unflyable : 0)
		| (options.unbuildable ? PathingMap::Flags::unbuildable : 0);
}

void TilePather::update_selected_icon() const {
	const PathingOptions options = pathing_options.at(selected_button->texture()->id);
	selected_button->create_icon(true, false, get_mask(options));
}

void TilePather::changed_tile(QAbstractButton* button) {
	TextureButton* tex_button = static_cast<TextureButton*>(button);
	ui.selectedTileLabel->setText("Tile: " + QString::fromStdString(tex_button->texture()->name));

	selected_button = tex_button;

	ui.unwalkable->setChecked(pathing_options[selected_button->texture()->id].unwalkable);
	ui.unflyable->setChecked(pathing_options[selected_button->texture()->id].unflyable);
	ui.unbuildable->setChecked(pathing_options[selected_button->texture()->id].unbuildable);
}

void TilePather::save_tiles() {
	// set of all tiles with modified pathing
	std::unordered_set<int> changed_ids;

	for (const auto& [tile_id, options] : pathing_options) {
		const uint8_t mask = get_mask(options);
		TerrainTexture* texture = map->tilesets.terrain_texture(tile_id);
		if (mask != texture->get_tile_pathing()) {
			changed_ids.insert(map->terrain.ground_texture_to_id(tile_id));
		}

		texture->override_pathing = (mask != texture->base_pathing) ? std::optional<uint8_t>(mask) : std::nullopt;
	}

	// if one of the tiles was modified, update the pathing map
	if (!changed_ids.empty()) {
		for (int i = 0; i < map->terrain.width; i++) {
			for (int j = 0; j < map->terrain.height; j++) {
				if (!changed_ids.contains(map->terrain.real_tile_texture(i, j))) {
					continue;
				}

				const int x0 = std::max(i * 4 - 2, 0);
				const int y0 = std::max(j * 4 - 2, 0);
				const int x1 = std::min(i * 4 + 2, map->pathing_map.width);
				const int y1 = std::min(j * 4 + 2, map->pathing_map.height);

				for (int y = y0; y < y1; y++) {
					for (int x = x0; x < x1; x++) {
						map->pathing_map.pathing_cells_static[y * map->pathing_map.width + x] =
							map->terrain.get_terrain_pathing(x, y, true, true, true, map->tilesets);
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

	emit map->terrain.tileset_changed();
	close();
}

TextureButton* TilePather::create_tex_button(const TerrainTexture* tex) {
	TextureButton* button = new TextureButton(tex, this);
	button->create_icon(true, false);
	button->setFixedSize(64, 64);
	button->setIconSize({64, 64});
	button->setCheckable(true);
	button->setToolTip(QString::fromUtf8(tex->name));
	return button;
}
