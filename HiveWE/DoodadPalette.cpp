#include "stdafx.h"

DoodadPalette::DoodadPalette(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.create();
	map.brush = &brush;

	for (auto&&[key, value] : world_edit_data.section("TileSets")) {
		const std::string tileset_key = value.front();
		ui.tileset->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DoodadCategories")) {
		const std::string tileset_key = value.front();
		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DestructibleCategories")) {
		const std::string tileset_key = value.front();
		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (int i = 1; i < map.doodads.doodads_slk.rows; i++) {
		QListWidgetItem* item = new QListWidgetItem(ui.doodads);
		item->setText(QString::fromStdString(map.doodads.doodads_slk.data("Name", i)));
		item->setData(Qt::UserRole, QString::fromStdString(map.doodads.doodads_slk.data("doodID", i)));
	}

	connect(ui.doodads, &QListWidget::itemClicked, this, &DoodadPalette::set_doodad);
}

DoodadPalette::~DoodadPalette() {
	map.brush = nullptr;
}

bool DoodadPalette::event(QEvent *e) {
	if (e->type() == QEvent::WindowActivate) {
		map.brush = &brush;
	}
	return QWidget::event(e);
}

void DoodadPalette::set_doodad(QListWidgetItem* item) {
	std::string id = item->data(Qt::UserRole).toString().toStdString();
	brush.mesh = map.doodads.get_mesh(id, 0);
}