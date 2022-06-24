#include "HiveWE.h"

#include <fstream>

#include <QFileDialog>
#include <QSettings>
#include <QObject>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QPushButton>
#include <QApplication>

#include <SOIL2/SOIL2.h>

#define STORMLIB_NO_AUTO_LINK
#include <StormLib.h>

import Hierarchy;

#include "TileSetter.h"
#include "MapInfoEditor.h"
#include "TerrainPalette.h"
#include "SettingsEditor.h"
#include "TilePather.h"
#include "Palette.h"
#include "TerrainPalette.h"
#include "DoodadPalette.h"
#include "UnitPalette.h"
#include "PathingPalette.h"
#include "ObjectEditor.h"
#include "ModelEditor.h"
#include "Camera.h"

#include "IconView.h"

import MPQ;

HiveWE::HiveWE(QWidget* parent) : QMainWindow(parent) {
	setAutoFillBackground(true);

	fs::path directory = find_warcraft_directory();
	QSettings settings;

	//TODO: these are out of hierarchy while QSettings have ICE in modules
	hierarchy.ptr = settings.value("flavour", "Retail").toString() != "Retail";
	hierarchy.hd = settings.value("hd", "True").toString() != "False";
	hierarchy.teen = settings.value("teen", "False").toString() != "False";
	QSettings war3reg("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III", QSettings::NativeFormat);
	hierarchy.local_files = war3reg.value("Allow Local Files", 0).toInt() != 0;
	while (!hierarchy.open_casc(directory)) {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
		if (directory == "") {
			exit(EXIT_SUCCESS);
		}
	}
	settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));

	// Place common.j and blizzard.j in the data folder. Required by JassHelper
	BinaryReader common = hierarchy.open_file("scripts/common.j");
	std::ofstream output("Data/Tools/common.j");
	output.write((char*)common.buffer.data(), common.buffer.size());
	BinaryReader blizzard = hierarchy.open_file("scripts/blizzard.j");
	std::ofstream output2("Data/Tools/blizzard.j");
	output2.write((char*)blizzard.buffer.data(), blizzard.buffer.size());

	ui.setupUi(this);
	context = ui.widget;
	restore_window_state();

	world_edit_strings.load("UI/WorldEditStrings.txt");
	world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
	world_edit_data.load("UI/WorldEditData.txt");

	world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
	world_edit_data.substitute(world_edit_strings, "WorldEditStrings");

	connect(ui.ribbon->undo, &QPushButton::clicked, [&]() { 
		// ToDo: temporary, undoing should still allow a selection to persist
		if (map->brush) {
			map->brush->clear_selection();
		}
		map->terrain_undo.undo(); 
	});
	connect(ui.ribbon->redo, &QPushButton::clicked, [&]() { 
		// ToDo: temporary, undoing should still allow a selection to persist
		if (map->brush) {
			map->brush->clear_selection();
		}
		map->terrain_undo.redo(); 
	});

	connect(new QShortcut(Qt::CTRL | Qt::Key_Z, this), &QShortcut::activated, ui.ribbon->undo, &QPushButton::click);
	connect(new QShortcut(Qt::CTRL | Qt::Key_Y, this), &QShortcut::activated, ui.ribbon->redo, &QPushButton::click);

	connect(ui.ribbon->units_visible, &QPushButton::toggled, [](bool checked) { map->render_units = checked; });
	connect(ui.ribbon->doodads_visible, &QPushButton::toggled, [](bool checked) { map->render_doodads = checked; });
	connect(ui.ribbon->pathing_visible, &QPushButton::toggled, [](bool checked) { map->render_pathing = checked; });
	connect(ui.ribbon->brush_visible, &QPushButton::toggled, [](bool checked) { map->render_brush = checked; });
	connect(ui.ribbon->lighting_visible, &QPushButton::toggled, [](bool checked) { map->render_lighting = checked; });
	connect(ui.ribbon->wireframe_visible, &QPushButton::toggled, [](bool checked) { map->render_wireframe = checked; });
	connect(ui.ribbon->debug_visible, &QPushButton::toggled, [](bool checked) { map->render_debug = checked; });
	connect(ui.ribbon->minimap_visible, &QPushButton::toggled, [&](bool checked) { (checked) ? minimap->show() : minimap->hide(); });

	connect(new QShortcut(Qt::CTRL | Qt::Key_U, this), &QShortcut::activated, ui.ribbon->units_visible, &QPushButton::click);
	connect(new QShortcut(Qt::CTRL | Qt::Key_D, this), &QShortcut::activated, ui.ribbon->doodads_visible, &QPushButton::click);
	connect(new QShortcut(Qt::CTRL | Qt::Key_P, this), &QShortcut::activated, ui.ribbon->pathing_visible, &QPushButton::click);
	connect(new QShortcut(Qt::CTRL | Qt::Key_L, this), &QShortcut::activated, ui.ribbon->lighting_visible, &QPushButton::click);
	connect(new QShortcut(Qt::CTRL | Qt::Key_T, this), &QShortcut::activated, ui.ribbon->wireframe_visible, &QPushButton::click);
	connect(new QShortcut(Qt::Key_F3, this), &QShortcut::activated, ui.ribbon->debug_visible, &QPushButton::click);

	// Reload theme
	connect(new QShortcut(Qt::Key_F5, this), &QShortcut::activated, [&]() {
		QSettings settings;
		QFile file("Data/Themes/" + settings.value("theme").toString() + ".qss");
		file.open(QFile::ReadOnly);
		QString StyleSheet = QLatin1String(file.readAll());

		qApp->setStyleSheet(StyleSheet);
	});

	connect(ui.ribbon->reset_camera, &QPushButton::clicked, [&]() { camera->reset(); });
	connect(ui.ribbon->switch_camera, &QPushButton::clicked, this, &HiveWE::switch_camera);
	setAutoFillBackground(true);

	connect(new QShortcut(Qt::Key_F1, this), &QShortcut::activated, ui.ribbon->switch_camera, &QPushButton::click);
	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), this), &QShortcut::activated, ui.ribbon->reset_camera, &QPushButton::click);

	connect(ui.ribbon->import_heightmap, &QPushButton::clicked, this, &HiveWE::import_heightmap);

	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_O), this, nullptr, nullptr, Qt::ApplicationShortcut), &QShortcut::activated, ui.ribbon->open_map_folder, &QPushButton::click);
	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_I), this, nullptr, nullptr, Qt::ApplicationShortcut), &QShortcut::activated, ui.ribbon->open_map_mpq, &QPushButton::click);
	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this, nullptr, nullptr, Qt::ApplicationShortcut), &QShortcut::activated, ui.ribbon->save_map, &QPushButton::click);
	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, nullptr, nullptr, Qt::ApplicationShortcut), &QShortcut::activated, ui.ribbon->save_map_as, &QPushButton::click);

	//connect(ui.ribbon->new_map, &QAction::triggered, this, &HiveWE::load);
	connect(ui.ribbon->open_map_folder, &QPushButton::clicked, this, &HiveWE::load_folder);
	connect(ui.ribbon->open_map_mpq, &QPushButton::clicked, this, &HiveWE::load_mpq);
	connect(ui.ribbon->save_map, &QPushButton::clicked, this, &HiveWE::save);
	connect(ui.ribbon->save_map_as, &QPushButton::clicked, this, &HiveWE::save_as);
	connect(ui.ribbon->export_mpq, &QPushButton::clicked, this, &HiveWE::export_mpq);
	connect(ui.ribbon->test_map, &QPushButton::clicked, this, &HiveWE::play_test);
	connect(ui.ribbon->settings, &QPushButton::clicked, [&]() { new SettingsEditor(this); });
	connect(ui.ribbon->switch_warcraft, &QPushButton::clicked, this, &HiveWE::switch_warcraft);
	connect(ui.ribbon->exit, &QPushButton::clicked, [&]() { QApplication::exit(); });

	connect(ui.ribbon->change_tileset, &QRibbonButton::clicked, [this]() { new TileSetter(this); });
	connect(ui.ribbon->change_tile_pathing, &QRibbonButton::clicked, [this]() { new TilePather(this); });

	connect(ui.ribbon->map_description, &QRibbonButton::clicked, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(0); });
	connect(ui.ribbon->map_loading_screen, &QRibbonButton::clicked, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(1); });
	connect(ui.ribbon->map_options, &QRibbonButton::clicked, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(2); });
	//connect(ui, &QAction::triggered, [&]() { (new MapInfoEditor(this))->ui.tabs->setCurrentIndex(3); });

	connect(new QShortcut(QKeySequence(Qt::Key_T), this, nullptr, nullptr, Qt::WindowShortcut), &QShortcut::activated, [&]() {
		open_palette<TerrainPalette>();
	});

	connect(ui.ribbon->terrain_palette, &QRibbonButton::clicked, [this]() {
		open_palette<TerrainPalette>();
	});

	connect(new QShortcut(QKeySequence(Qt::Key_D), this, nullptr, nullptr, Qt::WindowShortcut), &QShortcut::activated, [&]() {
		open_palette<DoodadPalette>();
	});
	connect(ui.ribbon->doodad_palette, &QRibbonButton::clicked, [this]() {
		open_palette<DoodadPalette>();
	});

	connect(new QShortcut(QKeySequence(Qt::Key_U), this, nullptr, nullptr, Qt::WindowShortcut), &QShortcut::activated, [&]() {
		open_palette<UnitPalette>();
	});

	connect(ui.ribbon->unit_palette, &QRibbonButton::clicked, [this]() {
		open_palette<UnitPalette>();
	});

	connect(new QShortcut(QKeySequence(Qt::Key_P), this, nullptr, nullptr, Qt::WindowShortcut), &QShortcut::activated, [&]() {
		ui.ribbon->pathing_visible->setChecked(true);
		open_palette<PathingPalette>();
	});

	connect(ui.ribbon->pathing_palette, &QRibbonButton::clicked, [this]() {
		ui.ribbon->pathing_visible->setChecked(true);
		open_palette<PathingPalette>();
	});

	setAutoFillBackground(true);

	connect(ui.ribbon->object_editor, &QRibbonButton::clicked, [this]() {
		bool created = false;
		window_handler.create_or_raise<ObjectEditor>(nullptr, created);
	});

	connect(ui.ribbon->model_editor, &QRibbonButton::clicked, [this]() {
		bool created = false;
		window_handler.create_or_raise<ModelEditor>(nullptr, created);
	});

	minimap->setParent(ui.widget);
	minimap->move(10, 10);
	minimap->show();

	connect(minimap, &Minimap::clicked, [](QPointF location) { camera->position = { location.x() * map->terrain.width, (1.0 - location.y()) * map->terrain.height, camera->position.z }; });
	map = new Map();
	connect(&map->terrain, &Terrain::minimap_changed, minimap, &Minimap::set_minimap);

	ui.widget->makeCurrent();
	map->load("Data/Test Map/");
	map->render_manager.resize_framebuffers(ui.widget->width(), ui.widget->height());
}

void HiveWE::load_folder() {
	QSettings settings;

	QString folder_name = QFileDialog::getExistingDirectory(this, "Open Map Directory",
															settings.value("openDirectory", QDir::current().path()).toString(),
															QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (folder_name == "") {
		return;
	}

	settings.setValue("openDirectory", folder_name);

	fs::path directory = folder_name.toStdString();

	if (!fs::exists(directory / "war3map.w3i")) {
		QMessageBox::information(this, "Opening map failed", "Opening the map failed. Select a map that is saved in folder mode or use the Open Map (MPQ) option");
		return;
	}

	QMessageBox* loading_box = new QMessageBox(QMessageBox::Icon::Information, "Loading Map", "Loading " + QString::fromStdString(directory.filename().string()));
	loading_box->show();

	delete map;
	map = new Map();

	connect(&map->terrain, &Terrain::minimap_changed, minimap, &Minimap::set_minimap);

	ui.widget->makeCurrent();
	map->load(directory);

	loading_box->close();
	delete loading_box;

	map->render_manager.resize_framebuffers(ui.widget->width(), ui.widget->height());
	setWindowTitle("HiveWE 0.7 - " + QString::fromStdString(map->filesystem_path.string()));
}

/// Load MPQ will extract all files from the archive in a user specified location
void HiveWE::load_mpq() {
	QSettings settings;

	// Choose an MPQ
	QString file_name = QFileDialog::getOpenFileName(this, "Open File",
													 settings.value("openDirectory", QDir::current().path()).toString(),
													 "Warcraft III Scenario (*.w3m *.w3x)");

	if (file_name == "") {
		return;
	}

	settings.setValue("openDirectory", file_name);

	fs::path mpq_path = file_name.toStdWString();

	mpq::MPQ mpq;
	bool opened = mpq.open(mpq_path);
	if (!opened) {
		QMessageBox::critical(this, "Opening map failed", "Opening the map archive failed. It might be opened in another program.");
		std::cout << GetLastError() << "\n";
		return;
	}

	fs::path unpack_location = QFileDialog::getExistingDirectory(this, "Choose Unpacking Location",
																 settings.value("openDirectory", QDir::current().path()).toString(),
																 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks)
								   .toStdString();

	if (unpack_location.empty()) {
		return;
	}

	fs::path final_directory = unpack_location / mpq_path.stem();

	try {
		fs::create_directory(final_directory);
	} catch (std::filesystem::filesystem_error& e) {
		QMessageBox::critical(this, "Error creating directory", "Failed to create the directory to unpack into with error:\n" + QString::fromStdString(e.what()), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
		return;
	}

	bool unpacked = mpq.unpack(final_directory);
	if (!unpacked) {
		QMessageBox::critical(this, "Unpacking failed", "There was an error unpacking the archive.");
		std::cout << GetLastError() << "\n";
		return;
	}

	// Load map
	delete map;
	map = new Map();

	connect(&map->terrain, &Terrain::minimap_changed, minimap, &Minimap::set_minimap);

	ui.widget->makeCurrent();
	map->load(final_directory);
	map->render_manager.resize_framebuffers(ui.widget->width(), ui.widget->height());
	setWindowTitle("HiveWE 0.7 - " + QString::fromStdString(map->filesystem_path.string()));
}

void HiveWE::save() {
	emit saving_initiated();
	map->save(map->filesystem_path);
};

void HiveWE::save_as() {
	QSettings settings;
	const QString directory = settings.value("openDirectory", QDir::current().path()).toString() + "/" + QString::fromStdString(map->name);

	fs::path file_name = QFileDialog::getExistingDirectory(this, "Choose Save Location",
														   settings.value("openDirectory", QDir::current().path()).toString(),
														   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdString();

	if (file_name.empty()) {
		return;
	}

	emit saving_initiated();
	if (fs::exists(file_name) && fs::equivalent(file_name, map->filesystem_path)) {
		map->save(map->filesystem_path);
	} else {
		fs::create_directories(file_name / map->name);

		hierarchy.map_directory = file_name / map->name;
		map->save(file_name / map->name);
	}

	setWindowTitle("HiveWE 0.7 - " + QString::fromStdString(map->filesystem_path.string()));
}

void HiveWE::export_mpq() {
	QSettings settings;
	const QString directory = settings.value("openDirectory", QDir::current().path()).toString() + "/" + QString::fromStdString(map->filesystem_path.filename().string());
	std::wstring file_name = QFileDialog::getSaveFileName(this, "Export Map to MPQ", directory, "Warcraft III Scenario (*.w3x)").toStdWString();

	if (file_name.empty()) {
		return;
	}

	fs::remove(file_name);

	emit saving_initiated();
	map->save(map->filesystem_path);

	unsigned long file_count = std::distance(fs::directory_iterator{ map->filesystem_path }, {});

	HANDLE handle;
	bool open = SFileCreateArchive(file_name.c_str(), MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES, file_count, &handle);
	if (!open) {
		QMessageBox::critical(this, "Exporting failed", "There was an error creating the archive.");
		std::cout << GetLastError() << "\n";
		return;
	}

	for (const auto& entry : fs::recursive_directory_iterator(map->filesystem_path)) {
		if (entry.is_regular_file()) {
			SFileAddFileEx(handle, entry.path().c_str(), entry.path().lexically_relative(map->filesystem_path).string().c_str(), MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME);
		}
	}
	SFileCompactArchive(handle, nullptr, false);
	SFileCloseArchive(handle);
}

void HiveWE::play_test() {
	emit saving_initiated();
	if (!map->save(map->filesystem_path)) {
		return;
	}
	QProcess* warcraft = new QProcess;
	const QString warcraft_path = QString::fromStdString(fs::canonical(hierarchy.root_directory / "x86_64" / "Warcraft III.exe").string());
	QStringList arguments;
	arguments << "-launch"
			  << "-loadfile" << QString::fromStdString(fs::canonical(map->filesystem_path).string());

	QSettings settings;
	if (settings.value("testArgs").toString() != "")
		arguments << settings.value("testArgs").toString().split(' ');

	warcraft->start(warcraft_path, arguments);
}

void HiveWE::closeEvent(QCloseEvent* event) {
	int choice = QMessageBox::question(this, "Do you want to quit?", "Are you sure you want to quit?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (choice == QMessageBox::Yes) {
		event->accept();
	} else {
		event->ignore();
	}
}

void HiveWE::resizeEvent(QResizeEvent* event) {
	QMainWindow::resizeEvent(event);
	QTimer::singleShot(0, [&] { save_window_state(); });
}

void HiveWE::moveEvent(QMoveEvent* event) {
	QMainWindow::moveEvent(event);
	QTimer::singleShot(0, [&] { save_window_state(); });
}

void HiveWE::switch_warcraft() {
	QSettings settings;
	fs::path directory;
	do {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
		if (directory == "") {
			directory = settings.value("warcraftDirectory").toString().toStdString();
		}
	} while (!hierarchy.open_casc(directory));

	if (directory != hierarchy.warcraft_directory) {
		settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));
	}
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

// ToDo move to terrain class?
void HiveWE::import_heightmap() {
	QMessageBox::information(this, "Heightmap information", "Will read the red channel and map this onto the range -16 to +16");
	QSettings settings;
	const QString directory = settings.value("openDirectory", QDir::current().path()).toString() + "/" + QString::fromStdString(map->filesystem_path.filename().string());

	QString file_name = QFileDialog::getOpenFileName(this, "Open Heightmap Image", directory);

	if (file_name == "") {
		return;
	}

	int width;
	int height;
	int channels;
	uint8_t* image_data = SOIL_load_image(file_name.toStdString().c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);

	if (width != map->terrain.width || height != map->terrain.height) {
		QMessageBox::warning(this, "Incorrect Image Size", QString("Image Size: %1x%2 does not match terrain size: %3x%4").arg(QString::number(width), QString::number(height), QString::number(map->terrain.width), QString::number(map->terrain.height)));
		return;
	}

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			map->terrain.corners[i][j].height = (image_data[((height - 1 - j) * width + i) * channels] - 128) / 16.f;
		}
	}

	map->terrain.update_ground_heights({ 0, 0, width, height });
	delete image_data;
}

void HiveWE::save_window_state() {
	QSettings settings;

	if (!isMaximized()) {
		settings.setValue("MainWindow/geometry", saveGeometry());
	}

	settings.setValue("MainWindow/maximized", isMaximized());
	settings.setValue("MainWindow/windowState", saveState());
}

void HiveWE::restore_window_state() {
	QSettings settings;

	if (settings.contains("MainWindow/windowState")) {
		restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
		restoreState(settings.value("MainWindow/windowState").toByteArray());
		if (settings.value("MainWindow/maximized").toBool()) {
			showMaximized();
		} else {
			showNormal();
		}
	} else {
		showMaximized();
	}
}

void HiveWE::set_current_custom_tab(QRibbonTab* tab, QString name) {
	if (current_custom_tab == tab) {
		return;
	}

	if (current_custom_tab != nullptr) {
		emit palette_changed(tab);
	}

	remove_custom_tab();
	current_custom_tab = tab;
	ui.ribbon->addTab(tab, name);
	ui.ribbon->setCurrentIndex(ui.ribbon->count() - 1);
}

void HiveWE::remove_custom_tab() {
	for (int i = 0; i < ui.ribbon->count(); i++) {
		if (ui.ribbon->widget(i) == current_custom_tab) {
			ui.ribbon->removeTab(i);
			current_custom_tab = nullptr;
			return;
		}
	}
}