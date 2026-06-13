#include "tile_setter.h"
#include "tile_picker.h"

#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

import std;
import ResourceManager;
import OpenGLUtilities;
import MapGlobal;
import Globals;
import Texture;
import <glm/glm.hpp>;

TileSetter::TileSetter(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	// widget_list->setViewMode(QListView::IconMode);
	// widget_list->setResizeMode(QListView::Adjust);
	// widget_list->setFlow(QListView::LeftToRight);
	// widget_list->setMovement(QListView::Snap);
	// widget_list->setGridSize(QSize(80, 80));
	// widget_list->setIconSize(QSize(64, 64));
	// widget_list->setUniformItemSizes(true);
	// widget_list->setWrapping(true);
	// widget_list->setDropIndicatorShown(true);
	// widget_list->setDragDropMode(QAbstractItemView::InternalMove);
	// widget_list->setAttribute(Qt::WA_TranslucentBackground);
	// widget_list->setStyleSheet("QListWidget { background: transparent; }");
	// widget_list->setAutoFillBackground(false);
	// ui.flowlayout_placeholder_1->addWidget(widget_list);

	ui.currentScrollAreaContent->setLayout(selected_layout);
	ui.availableScrollAreaContent->setLayout(available_layout);

	// create icons for current terrain textures
	for (const auto& i : map->terrain.tileset_ids) {
		const TerrainTexture* texture = map->tilesets.terrain_texture(i);
		if (!texture) {
			continue;
		}

		TextureButton* button = create_tex_button(texture);

		selected_layout->addWidget(button);
		selected_group->addButton(button);
	}

	// add tilesets to dropdown menus
	for (const auto& [key, tileset] : map->tilesets.tilesets()) {
		ui.tileset->addItem(QString::fromStdString(tileset.name), QString(QChar(key)));
		ui.baseTileset->addItem(QString::fromStdString(tileset.name), QString(QChar(key)));
	}

	// allow user to pick cliff base tiles
	ui.tileset->addItem("Cliff Base Tiles", "c");

	// update base tileset initial value
	int index = ui.baseTileset->findData(QString(map->terrain.tileset_id));
	if (index >= 0) {
		ui.baseTileset->setCurrentIndex(index);
	}

	set_scroll_view_height(ui.currentScrollArea, selected_layout);
	set_scroll_view_height(ui.availableScrollArea, available_layout);

	update_available_tiles();
	update_gui();

	connect(ui.tileset, &QComboBox::currentTextChanged, this, &TileSetter::update_available_tiles);
	connect(ui.resetTileset, &QPushButton::clicked, this, &TileSetter::reset_to_default);
	connect(ui.additionalAdd, &QPushButton::clicked, this, &TileSetter::add_tile);
	connect(ui.selectedRemove, &QPushButton::clicked, this, &TileSetter::remove_tile);
	connect(ui.selectedShiftLeft, &QPushButton::clicked, this, &TileSetter::shift_left);
	connect(ui.selectedShiftRight, &QPushButton::clicked, this, &TileSetter::shift_right);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TileSetter::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, [this](QAbstractButton*) {
		update_gui();
	});

	connect(available_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, [this](QAbstractButton*) {
		update_gui();
	});
	show();
}

/// computes the scroll area height so it shows limited number of rows before scrollbar appears
void TileSetter::set_scroll_view_height(QScrollArea* scroll_area, const FlowLayout* layout) const {
	int rowHeight = 64;

	const auto items = layout->items();
	if (!items.isEmpty()) {
		rowHeight = items.first()->sizeHint().height();
	}

	constexpr int rows = 2;

	const int height = rows * rowHeight + (rows - 1) * layout->vertical_spacing() + layout->contentsMargins().top()
		+ layout->contentsMargins().bottom() + scroll_area->frameWidth() * 2;

	scroll_area->setFixedHeight(height);
}

TextureButton* TileSetter::create_tex_button(const TerrainTexture* tex) {
	TextureButton* button = new TextureButton(tex, this);
	button->create_icon(true, true);
	button->setFixedSize(64, 64);
	button->setIconSize({64, 64});
	button->setCheckable(true);
	button->setToolTip(QString::fromUtf8(tex->name));
	return button;
}

void TileSetter::update_gui() const {
	// count the number of used terrain and cliff textures
	const int num_tex = selected_layout->count();
	int num_cliff = 0;
	for (const QAbstractButton* button : selected_group->buttons()) {
		auto* tb = static_cast<const TextureButton*>(button);
		const TerrainTexture* tex = tb->texture();

		if (tex && tex->cliff_type_id) {
			++num_cliff;
		}
	}

	// update current tiles (top part)
	if (const QAbstractButton* selected_tex = selected_group->checkedButton()) {
		auto* tb = static_cast<const TextureButton*>(selected_tex);
		const TerrainTexture* texture = tb->texture();

		ui.selectedTileLabel->setText(QString("Tile: %1").arg(QString::fromUtf8(texture->name)));

		const int index = selected_layout->indexOf(selected_tex);

		ui.selectedShiftLeft->setEnabled(index > 0);
		ui.selectedShiftRight->setEnabled(index >= 0 && index < num_tex - 1);

		ui.selectedRemove->setEnabled(!(texture->cliff_type_id && num_cliff <= 1));
	} else {
		ui.selectedTileLabel->setText("Tile:");
		ui.selectedShiftLeft->setEnabled(false);
		ui.selectedShiftRight->setEnabled(false);
		ui.selectedRemove->setEnabled(false);
	}
	ui.terrainTexCntLabel->setText(QString("Terrain Textures: %1 / 64").arg(num_tex));
	ui.cliffTexCntLabel->setText(QString("Cliff Textures: %1 / 15").arg(num_cliff));

	// update available tiles (bottom part)
	if (const QAbstractButton* available_tex = available_group->checkedButton()) {
		auto* tb = static_cast<const TextureButton*>(available_tex);
		const TerrainTexture* texture = tb->texture();

		ui.additionalTileLabel->setText(QString("Tile: %1").arg(QString::fromUtf8(texture->name)));

		bool already_added = false;
		for (const QAbstractButton* btn : selected_group->buttons()) {
			auto* sb = static_cast<const TextureButton*>(btn);

			if (sb->texture() == texture) {
				already_added = true;
				break;
			}
		}

		const bool cliff_limit = texture->cliff_type_id && (num_cliff >= 15);

		const bool disable = already_added || cliff_limit || (num_tex >= 64);
		ui.additionalAdd->setDisabled(disable);
	} else {
		ui.additionalTileLabel->setText("Tile:");
		ui.additionalAdd->setDisabled(true);
	}
}

void TileSetter::reset_to_default() {
	const auto result = QMessageBox::warning(
		this,
		"Warning",
		"Resetting to a base tileset will remove all custom terrain textures.\n"
		"Continue anyway?",
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);

	if (result != QMessageBox::Yes) {
		return;
	}

	const Tileset* tileset = map->tilesets.tileset(ui.baseTileset->currentData().toString().at(0).toLatin1());
	if (!tileset) {
		return;
	}

	// clear current terrain textures and replace them with the ones from base tileset
	for (QAbstractButton* button : selected_group->buttons()) {
		selected_layout->removeWidget(button);
		selected_group->removeButton(button);
		button->deleteLater();
	}

	for (const auto& tex_id : tileset->terrain_textures) {
		const TerrainTexture* tex = map->tilesets.terrain_texture(tex_id);
		if (!tex) {
			continue;
		}

		TextureButton* button = create_tex_button(tex);
		selected_layout->addWidget(button);
		selected_group->addButton(button);
	}

	update_gui();
}

void TileSetter::add_tile() {
	const auto available_button = available_group->checkedButton();
	if (!available_button) {
		return;
	}

	const auto* available_tex = static_cast<TextureButton*>(available_button);
	TextureButton* button = create_tex_button(available_tex->texture());

	selected_layout->addWidget(button);
	selected_group->addButton(button);

	update_gui();
}

void TileSetter::remove_tile() const {
	const auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	selected_layout->removeWidget(selected_button);
	selected_group->removeButton(selected_button);
	selected_button->deleteLater();

	update_gui();
}

void TileSetter::update_available_tiles() {
	available_layout->setEnabled(false);

	available_layout->clear();

	const std::string tileset = ui.tileset->currentData().toString().toStdString();

	for (const auto& [tex_id, texture] : map->tilesets.terrain_textures()) {
		if (tex_id.front() != tileset.front()) {
			continue;
		}

		TextureButton* button = create_tex_button(&texture);
		available_layout->addWidget(button);
		available_group->addButton(button);
	}

	available_layout->setEnabled(true);
	available_layout->invalidate();
}

void TileSetter::shift_left() const {
	const auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	const int index = selected_layout->indexOf(selected_button);
	selected_layout->move_widget(index - 1, selected_button);

	update_gui();
}

void TileSetter::shift_right() const {
	const auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	const int index = selected_layout->indexOf(selected_button);
	selected_layout->move_widget(index + 1, selected_button);

	update_gui();
}

void TileSetter::save_tiles() {
	std::vector<std::string> to_ids;
	for (const auto& item : selected_layout->items()) {
		const auto* btn = static_cast<const TextureButton*>(item->widget());
		to_ids.push_back(btn->texture()->id);
	}

	from_to_id.resize(map->terrain.tileset_ids.size());
	for (size_t i = 0; i < map->terrain.tileset_ids.size(); i++) {
		const std::string from_id = map->terrain.tileset_ids[i];

		const auto found = std::ranges::find(to_ids, from_id);
		if (found != to_ids.end()) {
			from_to_id[i] = found - to_ids.begin();
		} else {
			TilePicker replace_dialog(this, {from_id}, to_ids);
			connect(&replace_dialog, &TilePicker::tile_chosen, [&](const std::string& id, const std::string& to_id) {
				const auto tile_found = std::ranges::find(to_ids, to_id);
				from_to_id[i] = tile_found - to_ids.begin();
			});
			replace_dialog.exec();
		}
	}

	map->terrain.change_tileset(to_ids, from_to_id, map->tilesets);
	close();
}
