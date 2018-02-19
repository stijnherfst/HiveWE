#include "stdafx.h"

TilePicker::TilePicker(QWidget* parent, std::vector<std::string> from_ids, std::vector<std::string> to_ids) : QDialog(parent) {
	ui.setupUi(this);

	ui.flowlayout_placeholder_1->addLayout(from_layout);
	ui.flowlayout_placeholder_2->addLayout(to_layout);

	slk::SLK& slk = map.terrain.terrain_slk;
	for (auto&& i : from_ids) {
		auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i) + ".blp");
		auto icon = texture_to_icon(image->data, image->width, image->height);

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
		auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i) + ".blp");
		auto icon = texture_to_icon(image->data, image->width, image->height);

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

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePicker::accepted);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TilePicker::~TilePicker() {
}

void TilePicker::accepted() {
	std::string from_tile = from_group->checkedButton()->property("tileID").toString().toStdString();
	std::string to_tile = to_group->checkedButton()->property("tileID").toString().toStdString();
	emit tile_chosen(from_tile, to_tile);
	close();
	deleteLater();
}