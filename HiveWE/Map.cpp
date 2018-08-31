#include "stdafx.h"

Map::~Map() {
	hierarchy.map.close();
}

void Map::load(const fs::path& path) {
	hierarchy.map = mpq::MPQ(path);
	filesystem_path = fs::absolute(path);

	// Trigger strings
	if (hierarchy.map.file_exists("war3map.wts")) {
		if (auto t = hierarchy.map.file_open("war3map.wts").read2()) {
			BinaryReader war3map_wts(t.value());
			trigger_strings.load(war3map_wts);
		}
	}

	// Triggers (GUI and JASS)
	if (hierarchy.map.file_exists("war3map.wtg")) {
		BinaryReader war3map_wtg = BinaryReader(hierarchy.map.file_open("war3map.wtg").read());
		triggers.load(war3map_wtg);

		// Custom text triggers (JASS)
		if (hierarchy.map.file_exists("war3map.wct")) {
			BinaryReader war3map_wct = BinaryReader(hierarchy.map.file_open("war3map.wct").read());
			triggers.load_jass(war3map_wct);
		}
	}

	// Protection check
	is_protected = !hierarchy.map.file_exists("war3map.wtg");
	std::cout << "Protected: " << (is_protected ? "True\n" : " Possibly False\n");

	BinaryReader war3map_w3i(hierarchy.map.file_open("war3map.w3i").read());
	info.load(war3map_w3i);

	// Terrain
	BinaryReader war3map_w3e(hierarchy.map.file_open("war3map.w3e").read());
	bool success = terrain.load(war3map_w3e);
	if (!success) {
		return;
	}

//	doodads.tree.resize(terrain.width, terrain.height);
	units.tree.resize(terrain.width, terrain.height);

	// Pathing Map
	BinaryReader war3map_wpm(hierarchy.map.file_open("war3map.wpm").read());
	success = pathing_map.load(war3map_wpm, terrain);
	if (!success) {
		return;
	}

	// Imported Files
	if (hierarchy.map.file_exists("war3map.imp")) {
		BinaryReader war3map_imp = BinaryReader(hierarchy.map.file_open("war3map.imp").read());
		imports.load(war3map_imp);
	}
	if (hierarchy.map.file_exists("war3map.dir")) {
		BinaryReader war3map_dir = BinaryReader(hierarchy.map.file_open("war3map.dir").read());
		imports.load_dir_file(war3map_dir);
	}

	imports.poplate_uncategorized();

	// Doodads
	BinaryReader war3map_doo(hierarchy.map.file_open("war3map.doo").read());
	success = doodads.load(war3map_doo, terrain);

	if (hierarchy.map.file_exists("war3map.w3d")) {
		BinaryReader war3map_w3d = BinaryReader(hierarchy.map.file_open("war3map.w3d").read());
		doodads.load_doodad_modifications(war3map_w3d);
	}

	if (hierarchy.map.file_exists("war3map.w3b")) {
		BinaryReader war3map_w3b = BinaryReader(hierarchy.map.file_open("war3map.w3b").read());
		doodads.load_destructible_modifications(war3map_w3b);
	}

	doodads.create();

	// Units
	if (hierarchy.map.file_exists("war3mapUnits.doo")) {
		BinaryReader war3mapUnits_doo(hierarchy.map.file_open("war3mapUnits.doo").read());
		units_loaded = units.load(war3mapUnits_doo, terrain);

		if (units_loaded) {
			if (hierarchy.map.file_exists("war3map.w3u")) {
				BinaryReader war3map_w3u = BinaryReader(hierarchy.map.file_open("war3map.w3u").read());
				units.load_unit_modifications(war3map_w3u);
			}
			if (hierarchy.map.file_exists("war3map.w3t")) {
				BinaryReader war3map_w3t = BinaryReader(hierarchy.map.file_open("war3map.w3t").read());
				units.load_item_modifications(war3map_w3t);
			}
			units.create();
		}
	}

	camera->position = glm::vec3(terrain.width / 2, terrain.height / 2, 10);
	camera->reset();

	meshes.clear(); // ToDo this is not a nice way to do this

	loaded = true;
}

bool Map::save(const fs::path& path) const {
	std::error_code t;

	const fs::path complete_path = fs::absolute(path, t);
	if (complete_path != filesystem_path) {
		try {
			fs::copy_file(filesystem_path, complete_path, fs::copy_options::overwrite_existing);
		} catch (fs::filesystem_error& e) {
			QMessageBox msgbox;
			msgbox.setText(e.what());
			msgbox.exec();
			return false;
		}

		mpq::MPQ new_map(complete_path);

		std::swap(new_map, hierarchy.map);

		pathing_map.save();
		terrain.save();
		doodads.save();
		units.save();
		info.save();
		trigger_strings.save();

		imports.save();
		imports.save_dir_file();

		SFileCompactArchive(hierarchy.map.handle, nullptr, false);

		std::swap(new_map, hierarchy.map);
	} else {
		pathing_map.save();
		terrain.save();
		doodads.save();
		units.save();
		info.save();
		trigger_strings.save();

		imports.save();
		imports.save_dir_file();

		SFileCompactArchive(hierarchy.map.handle, nullptr, false);
	}
	return true;
}

void Map::play_test() {
	fs::path path = QDir::tempPath().toStdString() + "/temp.w3x";
	if (!save(path)) {
		return;
	}
	hierarchy.game_data.close();
	QProcess* warcraft = new QProcess;
	const QString warcraft_path = QString::fromStdString((hierarchy.warcraft_directory / "Warcraft III.exe").string());
	QStringList arguments;
	arguments << "-loadfile" << QString::fromStdString(path.string());

	warcraft->start("\"" + warcraft_path + "\"", arguments);
	warcraft->waitForFinished();
	hierarchy.game_data.open(hierarchy.warcraft_directory / "Data", 0);
}

void Map::render(int width, int height) {
	// While switching maps it may happen that render is called before loading has finished.
	if (!loaded) {
		return;
	}

	auto total_time_begin = std::chrono::high_resolution_clock::now();

	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl->glPolygonMode(GL_FRONT_AND_BACK, render_wireframe ? GL_LINE : GL_FILL);

	// Render Terrain
	auto begin = std::chrono::high_resolution_clock::now();

	terrain.render();

	auto end = std::chrono::high_resolution_clock::now();
	terrain_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	begin = std::chrono::high_resolution_clock::now();

	// Map mouse coordinates to world coordinates
	if (input_handler.mouse != input_handler.previous_mouse && input_handler.mouse.y() > 0) {
		glm::vec3 window = glm::vec3(input_handler.mouse.x(), height - input_handler.mouse.y(), 0);
		gl->glReadPixels(input_handler.mouse.x(), height - input_handler.mouse.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &window.z);
		input_handler.mouse_world = glm::unProject(window, camera->view, camera->projection, glm::vec4(0, 0, width, height));
	}

	end = std::chrono::high_resolution_clock::now();
	mouse_world_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render Doodads
	begin = std::chrono::high_resolution_clock::now();

	if (render_doodads) {
		doodads.render();
	}

	end = std::chrono::high_resolution_clock::now();
	doodad_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	// Render units
	if (units_loaded) {
		begin = std::chrono::high_resolution_clock::now();
		if (render_units) {
			units.render();
		}
		end = std::chrono::high_resolution_clock::now();
		unit_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;
	}

	if (render_brush && brush) {
		brush->render();
	}

	// Render all meshes
	begin = std::chrono::high_resolution_clock::now();

	for (auto&& i : meshes) {
		i->render();
	}

	end = std::chrono::high_resolution_clock::now();
	render_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;
	total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - total_time_begin).count() / 1'000'000.0;

	total_time_min = std::min(total_time, total_time_min);
	total_time_max = std::max(total_time, total_time_max);

	meshes.clear();
}