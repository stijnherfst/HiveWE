#include "tile_picker.h"

#include <QPushButton>

import std;
import ResourceManager;
import Texture;
import OpenGLUtilities;
import SLK;
import MapGlobal;

TilePicker::TilePicker(QWidget* parent, std::vector<std::string> from_ids, std::vector<std::string> to_ids) : QDialog(parent) {
	ui.setupUi(this);

	ui.flowlayout_placeholder_1->addLayout(from_layout);
	ui.flowlayout_placeholder_2->addLayout(to_layout);

	for (const auto& i : from_ids) {
		const TerrainTexture* texture = map->tilesets.terrain_texture(i);

		TextureButton* button = create_tex_button(texture);

		from_layout->addWidget(button);
		from_group->addButton(button);
	}

	for (const auto& i : to_ids) {
		const TerrainTexture* texture = map->tilesets.terrain_texture(i);

		TextureButton* button = create_tex_button(texture);

		to_layout->addWidget(button);
		to_group->addButton(button);
	}

	to_group->buttons().first()->setChecked(true);
	from_group->buttons().first()->setChecked(true);

	update_gui();

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePicker::completed);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(from_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton*) {
		update_gui();
	});

	connect(to_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton*) {
		update_gui();
	});
}

TextureButton* TilePicker::create_tex_button(const TerrainTexture* tex) {
	TextureButton* button = new TextureButton(tex, this);
	button->create_icon(true, true);
	button->setFixedSize(64, 64);
	button->setIconSize({64, 64});
	button->setCheckable(true);
	button->setToolTip(QString::fromUtf8(tex->name));
	return button;
}

void TilePicker::update_gui() const {
	if (const auto from = dynamic_cast<TextureButton*>(from_group->checkedButton()); from && from->hasTexture()) {
		ui.selectedTileLabel->setText("Tile: " + QString::fromStdString(from->texture()->name));
	} else {
		ui.selectedTileLabel->setText("Tile:");
	}

	if (const auto to = dynamic_cast<TextureButton*>(to_group->checkedButton()); to && to->hasTexture()) {
		ui.replacingTileLabel->setText("Tile: " + QString::fromStdString(to->texture()->name));
	} else {
		ui.replacingTileLabel->setText("Tile:");
	}
}

void TilePicker::completed() {
	const std::string from_tile = from_group->checkedButton()->property("tileID").toString().toStdString();
	const std::string to_tile = to_group->checkedButton()->property("tileID").toString().toStdString();
	emit tile_chosen(from_tile, to_tile);
	close();
}
