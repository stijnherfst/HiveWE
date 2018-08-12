#include "stdafx.h"

Map map;
ini::INI world_edit_strings;
ini::INI world_edit_game_strings;
ini::INI world_edit_data;
WindowHandler window_handler;

HiveWE::HiveWE(QWidget* parent) : QMainWindow(parent) {
	fs::path directory = find_warcraft_directory();
	while (!fs::exists(directory / "Data")) {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
		if (directory == "") {
			exit(EXIT_SUCCESS);
		}
	} ;
	QSettings settings;
	settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));
	hierarchy.warcraft_directory = directory;
	hierarchy.init();

	ui.setupUi(this);

	world_edit_strings.load("UI/WorldEditStrings.txt");
	world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
	world_edit_data.load("UI/WorldEditData.txt");

	world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
	world_edit_data.substitute(world_edit_strings, "WorldEditStrings");

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

	connect(ui.actionReset_Camera, &QAction::triggered, [&]() { camera->reset(); });
	connect(ui.actionSwitch_Camera, &QAction::triggered, this, &HiveWE::switch_camera);

	connect(ui.actionDescription, &QAction::triggered, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(0); });
	connect(ui.actionLoading_Screen, &QAction::triggered, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(1); });
	connect(ui.actionOptions, &QAction::triggered, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(2); });
	connect(ui.actionPreferences, &QAction::triggered, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(3); });

	connect(ui.actionTileSetter, &QAction::triggered, [this]() { new TileSetter(this); });
	connect(ui.actionChangeTilePathing, &QAction::triggered, [this]() { new TilePather(this); });

	connect(ui.actionEnforce_Water_Height_Limit, &QAction::triggered, [&](bool checked) { map.enforce_water_height_limits = checked; });


	connect(ui.actionPathing_Palette, &QAction::triggered, [this]() {
		auto palette = new PathingPallete(this);
		connect(this, &HiveWE::tileset_changed, [palette]() {
			palette->close();
		});
	});
	connect(ui.actionTerrain_Palette, &QAction::triggered, [this]() { new TerrainPalette(this); });
	connect(ui.actionDoodads_Palette, &QAction::triggered, [this]() { new DoodadPalette(this); });

	connect(ui.actionTrigger_Editor, &QAction::triggered, []() { window_handler.create_or_raise<TriggerEditor>(); });
	connect(ui.actionImport_Manager, &QAction::triggered, []() { window_handler.create_or_raise<ImportManager>(); });
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

void HiveWE::closeEvent(QCloseEvent* event) {
	//int choice = QMessageBox::question(this, "Do you want to quit?", "Are you sure you want to quit?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	//if (choice == QMessageBox::Yes) {
	//}
		event->accept();
}

void HiveWE::switch_camera() {
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
}