#include "Map.h"

#include <iostream>

#include <QMessageBox>
#include <QOpenGLFunctions_4_5_Core>

#include "Hierarchy.h"
#include "HiveWE.h"
#include "InputHandler.h"


void Map::load(const fs::path& path) {
	hierarchy.map_directory = path;
	filesystem_path = fs::absolute(path) / "";
	name = (*--(--filesystem_path.end())).string();

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
	if (hierarchy.map_file_exists("war3map.wts")) {
		BinaryReader war3map_wts = hierarchy.map_file_read("war3map.wts");
		trigger_strings.load(war3map_wts);
	}

	// Triggers (GUI and JASS)
	if (hierarchy.map_file_exists("war3map.wtg")) {
		BinaryReader war3map_wtg = hierarchy.map_file_read("war3map.wtg");
		triggers.load(war3map_wtg);

		// Custom text triggers (JASS)
		if (hierarchy.map_file_exists("war3map.wct")) {
			BinaryReader war3map_wct = hierarchy.map_file_read("war3map.wct");
			triggers.load_jass(war3map_wct);
		}
	}

	// Protection check
	is_protected = !hierarchy.map_file_exists("war3map.wtg");
	std::cout << "Protected: " << (is_protected ? "True\n" : " Possibly False\n");

	BinaryReader war3map_w3i = hierarchy.map_file_read("war3map.w3i");
	info.load(war3map_w3i);

	// Terrain
	BinaryReader war3map_w3e = hierarchy.map_file_read("war3map.w3e");
	bool success = terrain.load(war3map_w3e);
	if (!success) {
		return;
	}

	units.tree.resize(terrain.width, terrain.height);

	// Pathing Map
	BinaryReader war3map_wpm = hierarchy.map_file_read("war3map.wpm");
	success = pathing_map.load(war3map_wpm);
	if (!success) {
		return;
	}

	// Doodads
	BinaryReader war3map_doo = hierarchy.map_file_read("war3map.doo");
	success = doodads.load(war3map_doo, terrain);

	if (hierarchy.map_file_exists("war3map.w3d")) {
		BinaryReader war3map_w3d = hierarchy.map_file_read("war3map.w3d");
		doodads.load_doodad_modifications(war3map_w3d);
	}

	if (hierarchy.map_file_exists("war3map.w3b")) {
		BinaryReader war3map_w3b = hierarchy.map_file_read("war3map.w3b");
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
	if (hierarchy.map_file_exists("war3map.w3u")) {
		BinaryReader war3map_w3u = hierarchy.map_file_read("war3map.w3u");
		units.load_unit_modifications(war3map_w3u);
	}

	if (hierarchy.map_file_exists("war3map.w3t")) {
		BinaryReader war3map_w3t = hierarchy.map_file_read("war3map.w3t");
		units.load_item_modifications(war3map_w3t);
	}

	if (hierarchy.map_file_exists("war3mapUnits.doo")) {
		BinaryReader war3mapUnits_doo = hierarchy.map_file_read("war3mapUnits.doo");
		units_loaded = units.load(war3mapUnits_doo, terrain);

		if (units_loaded) {
			units.create();
		}
	}

	// Regions
	if (hierarchy.map_file_exists("war3map.w3r")) {
		BinaryReader war3map_w3r = hierarchy.map_file_read("war3map.w3r");
		regions.load(war3map_w3r);
	}

	// Cameras
	if (hierarchy.map_file_exists("war3map.w3c")) {
		BinaryReader war3map_w3c = hierarchy.map_file_read("war3map.w3c");
		cameras.load(war3map_w3c);
	}

	// Sounds
	if (hierarchy.map_file_exists("war3map.w3s")) {
		BinaryReader war3map_w3s = hierarchy.map_file_read("war3map.w3s");
		sounds.load(war3map_w3s);
	}

	camera->reset();

	loaded = true;
}

bool Map::save(const fs::path& path) {
	if (!fs::equivalent(path, filesystem_path)) {
		try {
			fs::copy(filesystem_path, fs::absolute(path), fs::copy_options::recursive);
		} catch (fs::filesystem_error& e) {
			QMessageBox msgbox;
			msgbox.setText(e.what());
			msgbox.exec();
			return false;
		}

	}

	filesystem_path = fs::absolute(path) / "";
	name = (*--(--filesystem_path.end())).string();

	pathing_map.save();
	terrain.save();
	doodads.save();
	units.save();
	info.save();
	trigger_strings.save();
	triggers.save();
	triggers.save_jass();
	triggers.generate_map_script();

	//imports.save();
	//imports.save_dir_file();

	return true;
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