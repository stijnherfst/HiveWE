#include "stdafx.h"

Map map;

HiveWE::HiveWE(QWidget* parent) : QMainWindow(parent) {
	fs::path directory = find_warcraft_directory();
	while (!fs::exists(directory / "War3x.mpq")) {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
	}
	QSettings settings;
	settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));
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
	connect(ui.actionLighting, &QAction::triggered, [&](bool checked) { map.render_lighting = checked; });
	connect(ui.actionWireframe, &QAction::triggered, [&](bool checked) { map.render_wireframe = checked; });
	connect(ui.actionFrame_Times, &QAction::triggered, [&](bool checked) { map.show_timings = checked; });
	connect(ui.actionReset_Camera, &QAction::triggered, [&](bool checked) { 
		camera->reset(); 
	});

	connect(ui.actionSwitch_Camera, &QAction::triggered, [&]() {
		if (camera == &ui.widget->tps_camera) {
			ui.widget->fps_camera.horizontal_angle = ui.widget->tps_camera.horizontal_angle;
			ui.widget->fps_camera.vertical_angle = ui.widget->tps_camera.vertical_angle;

			ui.widget->fps_camera.position = ui.widget->tps_camera.position;
			camera = &ui.widget->fps_camera;
			ui.actionDoodads->setEnabled(false);
		} else {
			ui.widget->tps_camera.horizontal_angle = ui.widget->fps_camera.horizontal_angle;
			ui.widget->tps_camera.vertical_angle = ui.widget->fps_camera.vertical_angle;

			ui.widget->tps_camera.position = ui.widget->fps_camera.position;
			camera = &ui.widget->tps_camera;
			ui.actionDoodads->setEnabled(true);
		}
		camera->update(0);
	});

	connect(ui.actionTileSetter, &QAction::triggered, [this]() { new TileSetter(this); });
	connect(ui.actionChangeTilePathing, &QAction::triggered, []() { new TilePather; });

	connect(ui.actionPathing_Palette, &QAction::triggered, [this]() {
		auto palette = new PathingPallete(this);
		connect(this, &HiveWE::tileset_changed, [palette]() {
			palette->close();
		});
	});

	connect(ui.actionTerrain_Palette, &QAction::triggered, [this]() { new TerrainPalette(this); });

	connect(ui.actionImport_Manager, &QAction::triggered, [this]() {
		if (auto manager = this->findChild<ImportManager*>(); !manager) {
			new ImportManager(this);
		} else {
			manager->activateWindow();
			manager->raise();
		}
	}); 
}

void HiveWE::load() {
	QSettings settings;

	QString file_name = QFileDialog::getOpenFileName(this, "Open File",
		settings.value("openDirectory", QDir::current().path()).toString(),
		"Warcraft III Scenario (*.w3x)");

	if (file_name != "") {
		settings.setValue("openDirectory", file_name);

		{ // Map falls out of scope so is cleaned before a new load
			Map new_map;
			std::swap(new_map, map);
		}
		map.load(file_name.toStdString());
	}
}

void HiveWE::save_as() {
	QSettings settings;
	const QString directory = settings.value("openDirectory", QDir::current().path()).toString() + "/" + QString::fromStdString(map.filesystem_path.filename().string());

	QString file_name = QFileDialog::getSaveFileName(this, "Save File",
		directory,
		"Warcraft III Scenario (*.w3x)");

	if (file_name != "") {
		map.save(file_name.toStdString());
	}
}