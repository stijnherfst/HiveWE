#include "stdafx.h"

Map::~Map() {
	hierarchy.map.close();
}

void Map::load(const fs::path& path) {
	hierarchy.map = mpq::MPQ(path);
	filesystem_path = fs::absolute(path);

	// Units
	units_slk = slk::SLK("Units/UnitData.slk");
	units_meta_slk = slk::SLK("Units/UnitMetaData.slk");

	units_slk.merge(slk::SLK("Units/UnitBalance.slk"));
	units_slk.merge(slk::SLK("Units/unitUI.slk"));
	units_slk.merge(slk::SLK("Units/UnitWeapons.slk"));
	units_slk.merge(slk::SLK("Units/UnitAbilities.slk"));

	units_slk.merge(ini::INI("Units/HumanUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/OrcUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/UndeadUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/NightElfUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/NeutralUnitFunc.txt"));
	units_slk.merge(ini::INI("Units/CampaignUnitFunc.txt"));

	units_slk.merge(ini::INI("Units/HumanUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/OrcUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/UndeadUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/NightElfUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/NeutralUnitStrings.txt"));
	units_slk.merge(ini::INI("Units/CampaignUnitStrings.txt"));

	abilities_slk = slk::SLK("Units/AbilityData.slk");
	abilities_slk.merge(ini::INI("Units/HumanAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/OrcAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/UndeadAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/NightElfAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/NeutralAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/ItemAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/CommonAbilityFunc.txt"));
	abilities_slk.merge(ini::INI("Units/CampaignAbilityFunc.txt"));

	abilities_slk.merge(ini::INI("Units/HumanAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/OrcAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/UndeadAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/NightElfAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/NeutralAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/ItemAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/CommonAbilityStrings.txt"));
	abilities_slk.merge(ini::INI("Units/CampaignAbilityStrings.txt"));

	// Items
	items_slk = slk::SLK("Units/ItemData.slk");
	items_slk.merge(ini::INI("Units/ItemFunc.txt"));
	items_slk.merge(ini::INI("Units/ItemStrings.txt"));

	// Doodads
	doodads_slk = slk::SLK("Doodads/Doodads.slk");
	doodads_meta_slk = slk::SLK("Doodads/DoodadMetaData.slk");
	
	doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	// Destructibles
	destructibles_slk = slk::SLK("Units/DestructableData.slk");
	destructibles_meta_slk = slk::SLK("Units/DestructableMetaData.slk");

	destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructibles_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	// Trigger strings
	if (hierarchy.map.file_exists("war3map.wts")) {
		if (auto t = hierarchy.map.file_open("war3map.wts").read2(); t) {
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

	units.tree.resize(terrain.width, terrain.height);

	// Pathing Map
	BinaryReader war3map_wpm(hierarchy.map.file_open("war3map.wpm").read());
	success = pathing_map.load(war3map_wpm);
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

	imports.populate_uncategorized();

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

	for (const auto& i : doodads.doodads) {
		if (!i.pathing) {
			continue;
		}

		pathing_map.blit_pathing_texture(i.position, 0, i.pathing);
	}
	pathing_map.upload_dynamic_pathing();

	// Units/Items
	if (hierarchy.map.file_exists("war3map.w3u")) {
		BinaryReader war3map_w3u = BinaryReader(hierarchy.map.file_open("war3map.w3u").read());
		units.load_unit_modifications(war3map_w3u);
	}

	if (hierarchy.map.file_exists("war3map.w3t")) {
		BinaryReader war3map_w3t = BinaryReader(hierarchy.map.file_open("war3map.w3t").read());
		units.load_item_modifications(war3map_w3t);
	}

	if (hierarchy.map.file_exists("war3mapUnits.doo")) {
		BinaryReader war3mapUnits_doo(hierarchy.map.file_open("war3mapUnits.doo").read());
		units_loaded = units.load(war3mapUnits_doo, terrain);

		if (units_loaded) {
			units.create();
		}
	}

	if (hierarchy.map.file_exists("war3map.w3r")) {
		BinaryReader war3map_w3r(hierarchy.map.file_open("war3map.w3r").read());
		regions.load(war3map_w3r);
	}

	if (hierarchy.map.file_exists("war3map.w3c")) {
		BinaryReader war3map_w3c(hierarchy.map.file_open("war3map.w3c").read());
		cameras.load(war3map_w3c);
	}

	if (hierarchy.map.file_exists("war3map.w3s")) {
		BinaryReader war3map_w3s(hierarchy.map.file_open("war3map.w3s").read());
		sounds.load(war3map_w3s);
	}

	camera->reset();

	loaded = true;

	triggers.generate_map_script();
}

bool Map::save(const fs::path& path, bool switch_working) {
	std::error_code t;

	mpq::MPQ new_map;

	// If the map is saved in another location we need to copy the map and switch our working W3X to that one
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

		new_map.open(complete_path);
		std::swap(new_map.handle, hierarchy.map.handle);
	}

	pathing_map.save();
	terrain.save();
	doodads.save();
	units.save();
	info.save();
	trigger_strings.save();
	triggers.save();
	triggers.save_jass();

	imports.save();
	imports.save_dir_file();

	bool result = SFileCompactArchive(hierarchy.map.handle, nullptr, false);
	if (!result) {
		std::cout << "Compacting error code: " << GetLastError() << "\n";
		QMessageBox::information(nullptr, "Compacting archive failed", "Compacting the map archive failed. This is not a crucial error, but the size of your map file will be slightly bigger");
	}

	// Switch back if we do not want to switch currently active W3X
	if (!switch_working && complete_path != filesystem_path) {
		std::swap(new_map.handle, hierarchy.map.handle);
	}
	
	new_map.close();

	return true;
}

void Map::play_test() {
	fs::path path = QDir::tempPath().toStdString() + "/temp.w3x";
	if (!save(path, false)) {
		return;
	}
	QProcess* warcraft = new QProcess;
	const QString warcraft_path = QString::fromStdString((hierarchy.warcraft_directory / "Warcraft III.exe").string());
	QStringList arguments;
	arguments << "-loadfile" << QString::fromStdString(path.string());

	QSettings settings;
	if (settings.value("testArgs").toString() != "")
		arguments << settings.value("testArgs").toString();

	warcraft->start("\"" + warcraft_path + "\"", arguments);
}

void Map::render(int width, int height) {
	// While switching maps it may happen that render is called before loading has finished.
	if (!loaded) {
		return;
	}

	total_time = (std::chrono::high_resolution_clock::now() - last_time).count() / 1'000'000.0;
	last_time = std::chrono::high_resolution_clock::now();


	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl->glPolygonMode(GL_FRONT_AND_BACK, render_wireframe ? GL_LINE : GL_FILL);

	// Render Terrain
	terrain.render();

	// Map mouse coordinates to world coordinates
	if (input_handler.mouse != input_handler.previous_mouse && input_handler.mouse.y() > 0) {
		glm::vec3 window = glm::vec3(input_handler.mouse.x(), height - input_handler.mouse.y(), 0);
		gl->glReadPixels(input_handler.mouse.x(), height - input_handler.mouse.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &window.z);
		input_handler.mouse_world = glm::unProject(window, camera->view, camera->projection, glm::vec4(0, 0, width, height));
	}

	// Render Doodads
	if (render_doodads) {
		doodads.render();
	}

	// Render units
	if (units_loaded) {
		if (render_units) {
			units.render();
		}
	}

	if (render_brush && brush) {
		brush->render();
	}

	// Render all meshes
	for (auto&& i : meshes) {
		i->render();
	}

	meshes.clear();
}