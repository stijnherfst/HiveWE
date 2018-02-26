#include "stdafx.h"

Map map;

HiveWE::HiveWE(QWidget *parent) : QMainWindow(parent) {
	fs::path directory = find_warcraft_directory();
	while (!fs::exists(directory / "War3Patch.mpq")) {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
	}
	hierarchy.warcraft_directory = directory;
	hierarchy.init();

	ui.setupUi(this);

	connect(ui.actionOpen, &QAction::triggered, this, &HiveWE::load);
	connect(ui.actionSave, &QAction::triggered, [&]() { map.save(map.filesystem_path); });
	connect(ui.actionSave_As, &QAction::triggered, this, &HiveWE::save_as);
	connect(ui.actionTest_Map, &QAction::triggered, [&]() { map.play_test(); });
	connect(ui.actionUnits, &QAction::triggered, [&](bool checked) { map.render_units = checked; });
	connect(ui.actionDoodads, &QAction::triggered, [&](bool checked) { map.render_doodads = checked; });
	connect(ui.actionPathing, &QAction::triggered, [&](bool checked) { map.render_pathing = checked; });
	connect(ui.actionBrush, &QAction::triggered, [&](bool checked) { map.render_brush = checked; });
	connect(ui.actionTileSetter, &QAction::triggered, [this]() { 
		TileSetter* tilesetter = new TileSetter(this); 
		dynamic_cast<GLWidget*>(ui.widget)->timer.stop(); 
		connect(tilesetter, &QDialog::finished, [this]() { 
			dynamic_cast<GLWidget*>(ui.widget)->timer.start(16);
		});
	});
	connect(ui.actionPathing_Pallete, &QAction::triggered, []() { new PathingPallete; });
}

void HiveWE::load() {
	QString file_name = QFileDialog::getOpenFileName(this, "Open File",
		QDir::current().path(),
		"Warcraft III Scenario (*.w3x)");

	if (file_name != "") {
		{ // Map falls out of scope so is cleaned before a new load
			Map new_map;
			std::swap(new_map, map);
		}
		map.load(file_name.toStdString());
	}
}

void HiveWE::save_as() {
	QString file_name = QFileDialog::getSaveFileName(this, "Save File",
		QDir::current().path() + "\\" + QString::fromStdString(map.filesystem_path.filename().string()),
		"Warcraft III Scenario (*.w3x)");

	if (file_name != "") {
		map.save(file_name.toStdString());
	}
}