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
	}
	QSettings settings;
	settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));
	hierarchy.warcraft_directory = directory;
	hierarchy.init();

	ui.setupUi(this);

	QRibbonTab* tab = new QRibbonTab(nullptr);


	// Clipbord
	QRibbonSection* home_section = new QRibbonSection;
	home_section->setText("Clipboard");

	QRibbonButton* copy = new QRibbonButton;
	copy->setIcon(QIcon("Data/Icons/Ribbon/copy32x32.ico"));
	copy->setText("Copy");
	home_section->addWidget(copy);

	QRibbonButton* paste = new QRibbonButton;
	paste->setIcon(QIcon("Data/Icons/Ribbon/paste32x32.ico"));
	paste->setText("Paste");
	home_section->addWidget(paste);

	QVBoxLayout* lay = new QVBoxLayout;

	QToolButton* but = new QToolButton;
	QToolButton* butt = new QToolButton;
	QToolButton* buttt = new QToolButton;

	lay->addWidget(but);
	lay->addWidget(butt);
	lay->addWidget(buttt);

	home_section->addLayout(lay);

	// View
	QRibbonTab* view_tab = new QRibbonTab(nullptr);

	// Visible section
	QRibbonSection* visible_section = new QRibbonSection;
	visible_section->setText("Visible");
	view_tab->add_section(visible_section);

	QRibbonButton* units_visible = new QRibbonButton;
	units_visible->setIcon(QIcon("Data/Icons/Ribbon/units32x32.png"));
	units_visible->setText("Units");
	units_visible->setCheckable(true);
	visible_section->addWidget(units_visible);

	QRibbonButton* doodads_visible = new QRibbonButton;
	doodads_visible->setIcon(QIcon("Data/Icons/Ribbon/doodads32x32.png"));
	doodads_visible->setText("Doodads");
	doodads_visible->setCheckable(true);
	visible_section->addWidget(doodads_visible);

	QRibbonButton* pathing_visible = new QRibbonButton;
	pathing_visible->setIcon(QIcon("Data/Icons/Ribbon/pathing32x32.png"));
	pathing_visible->setText("Pathing");
	pathing_visible->setCheckable(true);
	visible_section->addWidget(pathing_visible);

	QRibbonButton* brush_visible = new QRibbonButton;
	brush_visible->setIcon(QIcon("Data/Icons/Ribbon/brush32x32.png"));
	brush_visible->setText("Brush");
	brush_visible->setCheckable(true);
	visible_section->addWidget(brush_visible);

	QRibbonButton* lighting_visible = new QRibbonButton;
	lighting_visible->setIcon(QIcon("Data/Icons/Ribbon/lighting32x32.png"));
	lighting_visible->setText("Lighting");
	lighting_visible->setCheckable(true);
	visible_section->addWidget(lighting_visible);

	QRibbonButton* wireframe_visible = new QRibbonButton;
	wireframe_visible->setIcon(QIcon("Data/Icons/Ribbon/wireframe32x32.png"));
	wireframe_visible->setText("Wireframe");
	wireframe_visible->setCheckable(true);
	visible_section->addWidget(wireframe_visible);

	QRibbonButton* debug_visible = new QRibbonButton;
	debug_visible->setIcon(QIcon("Data/Icons/Ribbon/debug32x32.png"));
	debug_visible->setText("Debug");
	debug_visible->setCheckable(true);
	visible_section->addWidget(debug_visible);

	// Camera section

	QRibbonSection* camera_section = new QRibbonSection;
	camera_section->setText("Camera");
	view_tab->add_section(camera_section);

	QRibbonButton* switch_camera = new QRibbonButton;
	switch_camera->setIcon(QIcon("Data/Icons/Ribbon/switch32x32.png"));
	switch_camera->setText("Switch");
	camera_section->addWidget(switch_camera);

	QRibbonButton* reset_camera = new QRibbonButton;
	reset_camera->setIcon(QIcon("Data/Icons/Ribbon/reset32x32.png"));
	reset_camera->setText("Reset");
	camera_section->addWidget(reset_camera);

	//QRibbonSection* section = new QRibbonSection(nullptr);
	//QFormLayout* la = new QFormLayout;
	//QSpinBox* sp1 = new QSpinBox;
	//QSpinBox* sp2 = new QSpinBox;
	//la->addRow("Minimum", sp1);
	//la->addRow("Maximum", sp2);

	//tab->add_section(section);
	tab->add_section(home_section);
	//QWidget* editpage = new QWidget;
	//section->layoutt->insertLayout(0, la);

	ui.ribbon->addTab(tab, "Home");
	ui.ribbon->addTab(view_tab, "View");

	

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