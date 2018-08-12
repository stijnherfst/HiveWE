#include "stdafx.h"

DoodadPalette::DoodadPalette(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	for (auto&&[key, value] : world_edit_data.section("TileSets")) {
		//const std::string tileset_key = split(value, ',').front();

		const std::string tileset_key = value.front();
		ui.tileset->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DoodadCategories")) {
		//const std::string tileset_key = split(value, ',').front();
		const std::string tileset_key = value.front();

		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DestructibleCategories")) {
		//const std::string tileset_key = split(value, ',').front();
		const std::string tileset_key = value.front();

		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (int i = 1; i < map.doodads.doodads_slk.rows; i++) {
		ui.doodads->addItem(QString::fromStdString(map.doodads.doodads_slk.data("Name", i)));
	}

}

DoodadPalette::~DoodadPalette() {
}