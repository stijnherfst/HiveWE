#include "tile_setter.h"
#include "tile_picker.h"

#include "globals.h"
#include <map_global.h>

#include <QPushButton>
#include <QLabel>

#include <string>

import ResourceManager;
import OpenGLUtilities;

TileSetter::TileSetter(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	ui.flowlayout_placeholder_1->addLayout(selected_layout);
	ui.flowlayout_placeholder_2->addLayout(available_layout);

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

	for (auto&& [key, value] : world_edit_data.section("TileSets")) {
//		const std::string tileset_key = split(value, ',').front();
		ui.tileset->addItem(QString::fromStdString(value[0]), QString::fromStdString(key));
	}

	update_available_tiles();

	connect(ui.tileset, &QComboBox::currentTextChanged, this, &TileSetter::update_available_tiles);
	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TileSetter::existing_tile_clicked);
	connect(available_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TileSetter::available_tile_clicked);
	connect(ui.additionalAdd, &QPushButton::clicked, this, &TileSetter::add_tile);
	connect(ui.selectedRemove, &QPushButton::clicked, this, &TileSetter::remove_tile);
	connect(ui.selectedShiftLeft, &QPushButton::clicked, this, &TileSetter::shift_left);
	connect(ui.selectedShiftRight, &QPushButton::clicked, this, &TileSetter::shift_right);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TileSetter::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TileSetter::add_tile() const {
	const auto available_button = available_group->checkedButton();
	if (!available_button) {
		return;
	}

	QPushButton* button = new QPushButton;
	button->setIcon(available_button->icon());
	button->setFixedSize(64, 64);
	button->setIconSize({ 64, 64 });
	button->setCheckable(true);
	button->setProperty("tileID", available_button->property("tileID"));
	button->setProperty("tileName", available_button->property("tileName"));

	selected_layout->addWidget(button);
	selected_group->addButton(button);

	if (selected_group->buttons().size() >= 16) {
		ui.additionalAdd->setDisabled(true);
	}
}

void TileSetter::remove_tile() const {
	auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	selected_layout->removeWidget(selected_button);
	selected_group->removeButton(selected_button);
	selected_button->deleteLater();

	ui.additionalAdd->setEnabled(true);
}

void TileSetter::update_available_tiles() const {
	available_layout->clear();

	std::string tileset = ui.tileset->currentData().toString().toStdString();

	slk::SLK& slk = map->terrain.terrain_slk;
	for (auto&&[key, value] : map->terrain.terrain_slk.row_headers) {
		if (key.front() != tileset.front()) {
			continue;
		}

		const auto image = resource_manager.load<Texture>(slk.data("dir", key) + "\\" + slk.data("file", key));
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({ 64, 64 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(key));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", key)));

		available_layout->addWidget(button);
		available_group->addButton(button);
	}
}

void TileSetter::existing_tile_clicked(QAbstractButton* button) const {
	ui.selectedTileLabel->setText("Tile: " + button->property("tileName").toString());

	const int index = selected_layout->indexOf(button);
	ui.selectedShiftLeft->setEnabled(index != 0);
	ui.selectedShiftRight->setEnabled(index != selected_layout->count() - 1);

	// Check if cliff tile
	const std::string tile_id = button->property("tileID").toString().toStdString();
	auto& cliff_tiles = map->terrain.cliff_to_ground_texture;
	if (map->terrain.ground_texture_to_id.count(tile_id)) {
		const auto is_cliff_tile = std::find(cliff_tiles.begin(), cliff_tiles.end(), map->terrain.ground_texture_to_id[tile_id]);

		ui.selectedRemove->setEnabled(is_cliff_tile == cliff_tiles.end());
	}
}

void TileSetter::available_tile_clicked(QAbstractButton* button) const {
	ui.additionalTileLabel->setText("Tile: " + button->property("tileName").toString());

	// Check if tile was already in existing/modified tileset
	bool tile_already_added = false;
	for (auto&& i : selected_group->buttons()) {
		if (i->property("tileID") == button->property("tileID")) {
			tile_already_added = true;
		}
	}
	tile_already_added |= selected_group->buttons().size() >= 16;
	ui.additionalAdd->setDisabled(tile_already_added);
}

void TileSetter::shift_left() const {
	const auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	const int index = selected_layout->indexOf(selected_button);
	selected_layout->move_widget(index - 1, selected_button);

	if (index - 1 == 0) {
		ui.selectedShiftLeft->setEnabled(false);
	}

	ui.selectedShiftRight->setEnabled(true);
}

void TileSetter::shift_right() const {
	const auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	const int index = selected_layout->indexOf(selected_button);
	selected_layout->move_widget(index + 1, selected_button);

	if (index + 1 == selected_layout->count() - 1) {
		ui.selectedShiftRight->setEnabled(false);
	}

	ui.selectedShiftLeft->setEnabled(true);
}

void TileSetter::save_tiles() {
	std::vector<std::string> to_ids;
	for (auto&& j : selected_layout->items()) {
		to_ids.push_back(j->widget()->property("tileID").toString().toStdString());
	}

	from_to_id.resize(map->terrain.tileset_ids.size());
	for (size_t i = 0; i < map->terrain.tileset_ids.size(); i++) {
		const std::string from_id = map->terrain.tileset_ids[i];

		const auto found = std::find(to_ids.begin(), to_ids.end(), from_id);
		if (found != to_ids.end()) {
			from_to_id[i] =  found - to_ids.begin();
		} else {
			TilePicker replace_dialog(this, { from_id }, to_ids);
			connect(&replace_dialog, &TilePicker::tile_chosen, [&](std::string id, std::string to_id) {
				auto tile_found = std::find(to_ids.begin(), to_ids.end(), to_id);
				from_to_id[i] = tile_found - to_ids.begin();
			});
			replace_dialog.exec();
		}
	}

	map->terrain.change_tileset(to_ids, from_to_id);
	close();
}