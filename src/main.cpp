#define MI_MALLOC_OVERRIDE
#include <mimalloc.h>

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QTimer>

#include "main_window/hivewe.h"
#include "render/vulkan_test_modes.h"
#include "DockManager.h"

#ifdef WIN32
// To force HiveWE to run on the discrete GPU if available
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

#include <tracy/Tracy.hpp>

import std;
import Map;
import MapGlobal;
import Camera;
import <glm/glm.hpp>;
import Timer;
import Globals;
import Utilities;
import Hierarchy;
import BinaryReader;
import ThreadPool;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	ZoneScopedN("main");

	Timer start_timer;

	QCoreApplication::setOrganizationName("HiveWE");
	QCoreApplication::setApplicationName("HiveWE");

	QLocale::setDefault(QLocale("en_US"));

	// Create a dark palette
	// For some magically unknown reason Qt draws Qt::white text as black, so we use QColor(255, 254, 255) instead
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::WindowText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
	darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Text, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::PlaceholderText, Qt::gray);
	darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Dark, QColor(35, 35, 35));
	darkPalette.setColor(QPalette::Shadow, QColor(20, 20, 20));
	darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::ButtonText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
	darkPalette.setColor(QPalette::HighlightedText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

	QApplication::setPalette(darkPalette);
	QApplication::setStyle("Fusion");

	QApplication a(argc, argv);

	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting);
	ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton);
	ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility);
	ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize);
	ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab);

	QSettings settings;
	QFile file("data/themes/" + settings.value("theme", "Dark").toString() + ".qss");
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Error: Reading theme failed:" << file.error() << ": " << file.errorString();
		return -1;
	}

	a.setStyleSheet(QLatin1String(file.readAll()));

	const auto load_files = [] {
		// Place common.j and blizzard.j in the data folder. Required by JassHelper
		BinaryReader common = hierarchy.open_file("scripts/common.j").value();
		std::ofstream output("data/tools/common.j");
		output.write(reinterpret_cast<char*>(common.buffer.data()), common.buffer.size());
		BinaryReader blizzard = hierarchy.open_file("scripts/blizzard.j").value();
		std::ofstream output2("data/tools/blizzard.j");
		output2.write(reinterpret_cast<char*>(blizzard.buffer.data()), blizzard.buffer.size());

		world_edit_strings.load("UI/WorldEditStrings.txt");
		world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
		world_edit_data.load("UI/WorldEditData.txt");

		world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
		world_edit_data.substitute(world_edit_strings, "WorldEditStrings");
	};

	bool is_casc_open = false;
	const auto casc_future = std::async(std::launch::async, [&]() {
		const fs::path directory = find_warcraft_directory();

		is_casc_open = hierarchy.open_casc(directory);
		if (is_casc_open) {
			load_files();
		}
	});

	thread_pool.init();

	casc_future.wait();

	if (!is_casc_open) {
		fs::path directory = find_warcraft_directory();

		while (!hierarchy.open_casc(directory)) {
			directory = QFileDialog::getExistingDirectory(nullptr, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
			if (directory == "") {
				exit(EXIT_SUCCESS);
			}
		}
		settings.setValue("warcraftDirectory", QString::fromStdString(directory.string()));

		load_files();
	}

	if (QCoreApplication::arguments().contains("--vulkan-model-grid")) {
		return run_vulkan_model_grid_test();
	}
	if (QCoreApplication::arguments().contains("--vulkan-model-editor")) {
		return run_vulkan_model_editor_test();
	}

	HiveWE w;

	std::println("Application start: {}ms", start_timer.elapsed_ms());

	// `--map <path>` opens another folder map at startup; `--quit-after <seconds>` lets the map view run unattended for testing
	const QStringList arguments = QCoreApplication::arguments();
	if (const qsizetype index = arguments.indexOf("--map"); index >= 0 && index + 1 < arguments.size()) {
		w.load_map(arguments[index + 1].toStdWString());
	} else {
		w.load_map("data/test map/");
	}
	// `--camera x,y[,distance]` or `--camera cliffs` points the camera somewhere specific for testing
	if (const qsizetype index = arguments.indexOf("--camera"); index >= 0 && index + 1 < arguments.size() && map) {
		const QString value = arguments[index + 1];
		if (value == "cliffs") {
			if (!map->terrain.cliffs.empty()) {
				const glm::ivec3 cliff = map->terrain.cliffs[map->terrain.cliffs.size() / 2];
				camera.position.x = static_cast<float>(cliff.x);
				camera.position.y = static_cast<float>(cliff.y);
			}
		} else {
			const QStringList parts = value.split(',');
			if (parts.size() >= 2) {
				camera.position.x = parts[0].toFloat();
				camera.position.y = parts[1].toFloat();
			}
			if (parts.size() >= 3) {
				camera.distance = parts[2].toFloat();
			}
		}
	}
	if (const qsizetype index = arguments.indexOf("--quit-after"); index >= 0 && index + 1 < arguments.size()) {
		// exit() rather than quit(): quit() asks the main window to close, which prompts for confirmation
		QTimer::singleShot(arguments[index + 1].toInt() * 1000, [] {
			QCoreApplication::exit(0);
		});
	}
	// `--render-flags a,b,...` turns on map view toggles (wireframe, debug, pathing, regions) for testing
	if (const qsizetype index = arguments.indexOf("--render-flags"); index >= 0 && index + 1 < arguments.size() && map) {
		for (const QString& flag : arguments[index + 1].split(',')) {
			if (flag == "wireframe") {
				map->render_wireframe = true;
			} else if (flag == "debug") {
				map->render_debug = true;
			} else if (flag == "pathing") {
				map->render_pathing = true;
			} else if (flag == "regions") {
				map->render_regions = true;
			}
		}
	}
	// `--pick-test` centers the camera on the first unit, then the first doodad, picks at their projected
	// screen positions and prints whether the expected object came back, exercising the picking path
	if (arguments.contains("--pick-test")) {
		const auto screen_position = [](const glm::vec3 world) {
			const glm::vec4 clip = camera.projection_view * glm::vec4(world, 1.f);
			const glm::vec2 ndc = glm::vec2(clip) / clip.w;
			const glm::vec2 size = glm::vec2(map->render_manager.viewport_size());
			return glm::vec2((ndc.x + 1.f) * 0.5f * size.x, (1.f - ndc.y) * 0.5f * size.y);
		};
		// Start locations are not pickable
		const auto first_unit = [] {
			const auto& units = map->units.units;
			const auto it = std::ranges::find_if(units, [](const auto& unit) {
				return unit.id != "sloc";
			});
			return it == units.end() ? std::optional<size_t>() : std::optional<size_t>(it - units.begin());
		};
		QTimer::singleShot(3000, [=] {
			if (map && first_unit()) {
				camera.position = map->units.units[*first_unit()].position;
			}
		});
		QTimer::singleShot(4000, [=] {
			if (!map || !first_unit()) {
				std::println("Pick test: no units");
				return;
			}
			const size_t expected = *first_unit();
			const glm::vec2 at = screen_position(map->units.units[expected].position);
			input_handler.mouse = at;
			const auto unit = map->render_manager.pick_unit_id_under_mouse(map->units, at);
			std::println("Pick test unit at {}x{}: expected {}, got {}", at.x, at.y, expected, unit ? std::to_string(*unit) : "none");
			if (!map->doodads.doodads.empty()) {
				camera.position = map->doodads.doodads.front().position;
			}
		});
		QTimer::singleShot(5000, [=] {
			if (!map || map->doodads.doodads.empty()) {
				std::println("Pick test: no doodads");
				return;
			}
			const glm::vec2 at = screen_position(map->doodads.doodads.front().position);
			input_handler.mouse = at;
			const auto doodad = map->render_manager.pick_doodad_id_under_mouse(map->doodads, at);
			std::println("Pick test doodad at {}x{}: expected 0, got {}", at.x, at.y, doodad ? std::to_string(*doodad) : "none");
			for (const size_t i : {size_t {0}, doodad.value_or(0)}) {
				const auto& d = map->doodads.doodads[i];
				std::println("  doodad {} {} at {:.2f} {:.2f} {:.2f}", i, d.id, d.position.x, d.position.y, d.position.z);
			}
		});
	}

	return QApplication::exec();
}
