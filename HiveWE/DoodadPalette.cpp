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
		const std::string text = value.front();
		ui.type->addItem(QString::fromStdString(text), QString::fromStdString(key));
	}

	connect(ui.tileset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.doodads, &QListWidget::itemSelectionChanged, this, &DoodadPalette::selection_changed);

	update_list();
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

void DoodadPalette::update_list() {
	ui.doodads->clear();
	
	char selected_tileset = ui.tileset->currentData().toString().toStdString().front();
	std::string selected_category = ui.type->currentData().toString().toStdString();

	bool is_doodad = world_edit_data.key_exists("DoodadCategories", selected_category);
	slk::SLK& slk = is_doodad ? map.doodads.doodads_slk : map.doodads.destructibles_slk;

	for (int i = 1; i < slk.rows; i++) {
		// If the doodad belongs to this tileset
		std::string tilesets = slk.data("tilesets", i);
		if (tilesets != "*" && tilesets.find(selected_tileset) == std::string::npos) {
			continue;
		}

		// If the doodad belongs to this category
		std::string category = slk.data("category", i);
		if (category != selected_category) {
			continue;
		}

		std::string text = slk.data("Name", i);
		if (!is_doodad) {
			text += " " + map.doodads.destructibles_slk.data("EditorSuffix", i);
		}

		QListWidgetItem* item = new QListWidgetItem(ui.doodads);
		item->setText(QString::fromStdString(text));
		item->setData(Qt::UserRole, QString::fromStdString(slk.data(is_doodad ? "doodID" : "DestructableID", i)));
	}
}

void DoodadPalette::selection_changed() {
	if (ui.doodads->selectedItems().isEmpty()) {
		return;
	}
	QListWidgetItem* item = ui.doodads->selectedItems().first();

	brush.set_doodad(item->data(Qt::UserRole).toString().toStdString());
}