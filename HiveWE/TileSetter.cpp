#include "stdafx.h"

auto texture_to_icon = [](std::shared_ptr<Texture> image) {
	QImage temp_image = QImage(image->data, image->width, image->height, QImage::Format::Format_ARGB32);
	int size = image->height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
};

TileSetter::TileSetter(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	show();

	ui.flowlayout_placeholder_1->addLayout(selected_layout);
	ui.flowlayout_placeholder_2->addLayout(available_layout);

	ini::INI world_edit_data("UI/WorldEditData.txt");
	ini::INI world_edit_strings("UI/WorldEditGameStrings.txt");

	slk::SLK& slk = map.terrain.terrain_slk;
	for (auto&& i : map.terrain.tileset_ids) {
		auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i) + ".blp");
		auto icon = texture_to_icon(image);

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
		std::string tileset_key = split(value, ',').front();
		std::string name = world_edit_strings.data("WorldEditStrings", tileset_key);

		ui.tileset->addItem(QString::fromStdString(name), QString::fromStdString(key));
	}

	update_available_tiles();

	connect(ui.tileset, &QComboBox::currentTextChanged, this, &TileSetter::update_available_tiles);
	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.selectedTileLabel->setText("Tile: " + button->property("tileName").toString());
	});
	connect(available_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.additionalTileLabel->setText("Tile: " + button->property("tileName").toString());
	});
	connect(ui.additionalAdd, &QPushButton::clicked, this, &TileSetter::add_tile);
	connect(ui.selectedRemove, &QPushButton::clicked, this, &TileSetter::remove_tile);
	connect(ui.selectedShiftLeft, &QPushButton::clicked, this, &TileSetter::shift_left);
	connect(ui.selectedShiftRight, &QPushButton::clicked, this, &TileSetter::shift_right);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TileSetter::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TileSetter::~TileSetter() {
	deleteLater();
}

void TileSetter::update_available_tiles() {
	available_layout->clear();

	std::string tileset = ui.tileset->currentData().toString().toStdString();

	slk::SLK& slk = map.terrain.terrain_slk;
	for (auto&& [key, value] : map.terrain.terrain_slk.header_to_row) {
		if (key.front() != tileset.front()) {
			continue;
		}

		auto image = resource_manager.load<Texture>(slk.data("dir", key) + "\\" + slk.data("file", key) + ".blp");
		auto icon = texture_to_icon(image);

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
	resize(sizeHint());
}

void TileSetter::add_tile() {
	auto available_button = available_group->checkedButton();
	if (!available_button) {
		return;
	}

	// if the tile is already added
	for (auto&& i : selected_group->buttons()) {// selected_layout->children()) {
		if (i->property("tileID") == available_button->property("tileID")) {
			return;
		}
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

void TileSetter::remove_tile() {
	auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	selected_layout->removeWidget(selected_button);
	selected_group->removeButton(selected_button);
	selected_button->deleteLater();
}

void TileSetter::shift_left() {
	auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	int index = selected_layout->indexOf(selected_button);
	selected_layout->moveWidget(index - 1, selected_button);
}

void TileSetter::shift_right() {
	auto selected_button = selected_group->checkedButton();
	if (!selected_button) {
		return;
	}

	int index = selected_layout->indexOf(selected_button);
	selected_layout->moveWidget(index + 1, selected_button);
}

void TileSetter::save_tiles() {

}