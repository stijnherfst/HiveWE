#include "stdafx.h"

Map::~Map() {
	hierarchy.tileset.close();
	hierarchy.map.close();
}

void Map::load(fs::path path) {
	hierarchy.map = mpq::MPQ(path);
	filesystem_path = fs::system_complete(path);

	// Terrain
	BinaryReader war3map_w3e = BinaryReader(hierarchy.map.file_open("war3map.w3e").read());
	bool success = terrain.load(war3map_w3e);
	if (!success) {
		return;
	}

	// Pathing Map
	BinaryReader war3map_wpm = BinaryReader(hierarchy.map.file_open("war3map.wpm").read());
	success = pathing_map.load(war3map_wpm, terrain);
	if (!success) {
		return;
	}

	// Doodads
	BinaryReader war3map_doo = BinaryReader(hierarchy.map.file_open("war3map.doo").read());
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

	brush.create();

	camera.position = glm::vec3(terrain.width / 2, terrain.height / 2, 0);

	meshes.clear(); // ToDo
}

bool Map::save(fs::path path) {
	fs::path complete_path = fs::system_complete(path);
	if (complete_path != filesystem_path) {
		try {
			fs::copy_file(filesystem_path, complete_path, fs::copy_options::overwrite_existing);
		} catch (fs::filesystem_error& e) {
			QMessageBox Msgbox;
			Msgbox.setText(e.what());
			Msgbox.exec();
			return false;
		}

		mpq::MPQ new_map(complete_path);

		std::swap(new_map, hierarchy.map);

		pathing_map.save();
		terrain.save();

		std::swap(new_map, hierarchy.map);
	} else {
		pathing_map.save();
		terrain.save();
	}
	return true;
}

void Map::play_test() {
	if (!save("Data/Temporary/temp.w3x")) {
		return;
	}

	QProcess* warcraft = new QProcess;
	QString warcraft_path = QString::fromStdString((hierarchy.warcraft_directory / "Warcraft III.exe").string());
	QStringList arguments;
	arguments << "-loadfile" << QString::fromStdString(fs::system_complete("Data/Temporary/temp.w3x").string());

	warcraft->start("\"" + warcraft_path + "\"", arguments);
}

void Map::render() {
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	auto begin = std::chrono::high_resolution_clock::now();

	terrain.render();

	auto end = std::chrono::high_resolution_clock::now();
	terrain_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	begin = std::chrono::high_resolution_clock::now();

	if (render_doodads) {
		doodads.render();
	}

	end = std::chrono::high_resolution_clock::now();
	queue_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	if (render_brush) {
		brush.render(terrain);
	}

	begin = std::chrono::high_resolution_clock::now();

	for (auto&& i : meshes) {
		i->render();
	}

	end = std::chrono::high_resolution_clock::now();
	doodad_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1'000'000.0;

	meshes.clear();
}