#include "stdafx.h"

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
}

void Map::close() {
	hierarchy.tileset.close();
	hierarchy.map.close();
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

		std::swap(new_map, hierarchy.map);
	} else {
		pathing_map.save();
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
	terrain.render();

	if (render_doodads) {
		doodads.render();
	}

	if (render_brush) {
		brush.render(terrain);
	}
}