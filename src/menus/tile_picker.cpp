#include "tile_picker.h"

#include "ankerl/unordered_dense.h"
#include <QPushButton>

import ResourceManager;
import Texture;
import OpenGLUtilities;
import SLK;

#include <map_global.h>

TilePicker::TilePicker(QWidget* parent, std::vector<std::string> from_ids, std::vector<std::string> to_ids) : QDialog(parent) {
	ui.setupUi(this);

	ui.flowlayout_placeholder_1->addLayout(from_layout);
	ui.flowlayout_placeholder_2->addLayout(to_layout);

	slk::SLK& slk = map->terrain.terrain_slk;
	for (auto&& i : from_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i));
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({ 64, 64 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		from_layout->addWidget(button);
		from_group->addButton(button);
	}

	for (auto&& i : to_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i));
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({ 64, 64 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		to_layout->addWidget(button);
		to_group->addButton(button);
	}

	to_group->buttons().first()->setChecked(true);
	from_group->buttons().first()->setChecked(true);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePicker::completed);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	
	connect(from_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.selectedTileLabel->setText("Tile: " + button->property("tileName").toString());
	});

	connect(to_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.replacingTileLabel->setText("Tile: " + button->property("tileName").toString());
	});
}

void TilePicker::completed() {
	const std::string from_tile = from_group->checkedButton()->property("tileID").toString().toStdString();
	const std::string to_tile = to_group->checkedButton()->property("tileID").toString().toStdString();
	emit tile_chosen(from_tile, to_tile);
	close();
}