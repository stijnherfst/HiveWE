#include "stdafx.h"
#include "TilePathing.h"

TilePathing::TilePathing(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	show();

	ui.flowlayout_placeholder->addLayout(selected_layout);

	slk::SLK& slk = map.terrain.terrain_slk;
	for (auto&& i : map.terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "\\" + slk.data("file", i) + ".blp");
		const auto icon = texture_to_icon(image->data, image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(64, 64);
		button->setIconSize({ 64, 64 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		selected_layout->addWidget(button);
		selected_group->addButton(button);

		pathing_options.emplace(i, PathingOptions()); // ToDo make tile pathing options state for the map
	}

	selected_group->buttons().first()->setChecked(true);
	current_tile = selected_group->buttons().first()->property("tileID").toString().toStdString();

	connect(selected_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.selectedTileLabel->setText("Tile: " + button->property("tileName").toString());
		changed_tile(button);
	});

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &TilePathing::save_tiles);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(ui.replaceType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::replace; });
	connect(ui.addType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::add; });
	connect(ui.removeType, &QPushButton::clicked, [&]() { pathing_options[current_tile].operation = PathingOptions::Operation::remove; });

	connect(ui.walkable, &QPushButton::clicked, [&]() { pathing_options[current_tile].walkable = ui.walkable->isChecked(); });
	connect(ui.flyable, &QPushButton::clicked, [&]() { pathing_options[current_tile].flyable = ui.flyable->isChecked(); });
	connect(ui.buildable, &QPushButton::clicked, [&]() { pathing_options[current_tile].buildable = ui.buildable->isChecked(); });
}

TilePathing::~TilePathing() {
	deleteLater();
}

void TilePathing::changed_tile(QAbstractButton* button) {
	current_tile = button->property("tileID").toString().toStdString();

	ui.walkable->setChecked(pathing_options[current_tile].walkable);
	ui.flyable->setChecked(pathing_options[current_tile].flyable);
	ui.buildable->setChecked(pathing_options[current_tile].buildable);

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

void TilePathing::save_tiles() {
	
}