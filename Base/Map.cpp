#include "Map.h"

#include <iostream>

#include <QMessageBox>
#include <QOpenGLFunctions_4_5_Core>

#include "Hierarchy.h"
#include "HiveWE.h"
#include "InputHandler.h"
#include "Physics.h"
#include "Camera.h"

#include <fstream>
#include <bullet/btBulletDynamicsCommon.h>

void Map::load(const fs::path& path) {
	auto begin = std::chrono::steady_clock::now();

	hierarchy.map_directory = path;
	filesystem_path = fs::absolute(path) / "";
	name = (*--(--filesystem_path.end())).string();

	// ToDo So for the game data files we should actually load from _balance/custom_v0.w3mod/Units, _balance/custom_v1.w3mod/Units, _balance/melee_v0.w3mod/units or /Units depending on the Game Data set and Game Data Versions
	// Maybe just ignore RoC so we only need to choose between _balance/custom_v1.w3mod/Units and /Units
	// Maybe just force everyone to suck it up and use /Units

	// Units
	units_slk = slk::SLK("Units/UnitData.slk");
	units_meta_slk = slk::SLK("Data/Warcraft/UnitMetaData.slk", true);
	units_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	unit_editor_data = ini::INI("UI/UnitEditorData.txt");
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");
	// Have to substitute twice since some of the keys refer to other keys in the same file
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");

	units_slk.merge(ini::INI("Units/UnitSkin.txt"));
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
	abilities_meta_slk = slk::SLK("Units/AbilityMetaData.slk");
	abilities_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	abilities_slk.merge(ini::INI("Units/AbilitySkin.txt"));
	abilities_slk.merge(ini::INI("Units/AbilitySkinStrings.txt"));
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
	items_slk.merge(ini::INI("Units/ItemSkin.txt"));
	items_slk.merge(ini::INI("Units/ItemFunc.txt"));
	items_slk.merge(ini::INI("Units/ItemStrings.txt"));

	items_meta_slk = slk::SLK("Data/Warcraft/ItemMetaData.slk", true);
	items_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	// Doodads
	doodads_slk = slk::SLK("Doodads/Doodads.slk");
	doodads_meta_slk = slk::SLK("Doodads/DoodadMetaData.slk");
	doodads_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	doodads_slk.merge(ini::INI("Doodads/DoodadSkins.txt"));
	doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	// Destructables
	destructables_slk = slk::SLK("Units/DestructableData.slk");
	destructables_meta_slk = slk::SLK("Units/DestructableMetaData.slk");

	destructables_slk.merge(ini::INI("Units/DestructableSkin.txt"));
	destructables_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructables_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	upgrade_slk = slk::SLK("Units/UpgradeData.slk");
	upgrade_meta_slk = slk::SLK("Units/UpgradeMetaData.slk");
	upgrade_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	upgrade_slk.merge(ini::INI("Units/AbilitySkin.txt"));
	upgrade_slk.merge(ini::INI("Units/UpgradeSkin.txt"));
	upgrade_slk.merge(ini::INI("Units/HumanUpgradeFunc.txt"));
	upgrade_slk.merge(ini::INI("Units/OrcUpgradeFunc.txt"));
	upgrade_slk.merge(ini::INI("Units/UndeadUpgradeFunc.txt"));
	upgrade_slk.merge(ini::INI("Units/NightElfUpgradeFunc.txt"));
	upgrade_slk.merge(ini::INI("Units/NeutralUpgradeFunc.txt"));
	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeFunc.txt"));

	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/HumanUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/NeutralUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/NightElfUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/OrcUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/UndeadUpgradeStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/UpgradeSkinStrings.txt"));
	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeFunc.txt"));

	buff_slk = slk::SLK("Units/AbilityBuffData.slk");
	buff_meta_slk = slk::SLK("Units/AbilityBuffMetaData.slk");
	buff_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	buff_slk.merge(ini::INI("Units/AbilitySkin.txt"));
	buff_slk.merge(ini::INI("Units/AbilitySkinStrings.txt"));
	buff_slk.merge(ini::INI("Units/HumanAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/OrcAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/UndeadAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/NightElfAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/NeutralAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/ItemAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/CommonAbilityFunc.txt"));
	buff_slk.merge(ini::INI("Units/CampaignAbilityFunc.txt"));
	
	buff_slk.merge(ini::INI("Units/HumanAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/OrcAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/UndeadAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/NightElfAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/NeutralAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/ItemAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/CommonAbilityStrings.txt"));
	buff_slk.merge(ini::INI("Units/CampaignAbilityStrings.txt"));

	units_table = new TableModel(&units_slk, &units_meta_slk);
	items_table = new TableModel(&items_slk, &items_meta_slk);
	abilities_table = new TableModel(&abilities_slk, &abilities_meta_slk);
	doodads_table = new TableModel(&doodads_slk, &doodads_meta_slk);
	destructables_table = new TableModel(&destructables_slk, &destructables_meta_slk);
	upgrade_table = new TableModel(&upgrade_slk, &upgrade_meta_slk);
	buff_table = new TableModel(&buff_slk, &buff_meta_slk);

	auto delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "SLK loading: " << delta << "ms\n";


//	physics.initialize();

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

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Trigger loading: " << delta << "ms\n";

	// Protection check
	is_protected = !hierarchy.map_file_exists("war3map.wtg");
	std::cout << "Protected: " << (is_protected ? "True\n" : " Possibly False\n");

	BinaryReader war3map_w3i = hierarchy.map_file_read("war3map.w3i");
	info.load(war3map_w3i);

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Info loading: " << delta << "ms\n";

	// Terrain
	BinaryReader war3map_w3e = hierarchy.map_file_read("war3map.w3e");
	bool success = terrain.load(war3map_w3e);
	if (!success) {
		return;
	}

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Terrain loading: " << delta << "ms\n";

	units.tree.resize(terrain.width, terrain.height);

	// Pathing Map
	BinaryReader war3map_wpm = hierarchy.map_file_read("war3map.wpm");
	success = pathing_map.load(war3map_wpm);
	if (!success) {
		return;
	}

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Pathing loading: " << delta << "ms\n";

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

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Doodad loading: " << delta << "ms\n";

	for (const auto& i : doodads.doodads) {
		if (!i.pathing) {
			continue;
		}

		if (doodads_slk.row_header_exists(i.id)) {
			continue;
		}

		pathing_map.blit_pathing_texture(i.position, glm::degrees(i.angle) + 90, i.pathing);
	}
	pathing_map.upload_dynamic_pathing();

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Doodad blitting: " << delta << "ms\n";

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

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Unit loading: " << delta << "ms\n";

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

	delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000;
	begin = std::chrono::steady_clock::now();
	std::cout << "Regions/cameras/sounds loading: " << delta << "ms\n";

	camera->reset();

	loaded = true;
}

bool Map::save(const fs::path& path) {
	if (!fs::equivalent(path, filesystem_path)) {
		try {
			fs::copy(filesystem_path, fs::absolute(path), fs::copy_options::recursive);
		} catch (fs::filesystem_error & e) {
			QMessageBox msgbox;
			msgbox.setText(e.what());
			msgbox.exec();
			return false;
		}

		filesystem_path = fs::absolute(path) / "";
		name = (*--(--filesystem_path.end())).string();
	}

	pathing_map.save();
	terrain.save();
	doodads.save();
	units.save();
	info.save();
	trigger_strings.save();
	triggers.save();
	triggers.save_jass();
	triggers.generate_map_script();

	return true;
}

void Map::update(double delta, int width, int height) {
	if (loaded) {
		camera->update(delta);

		terrain.current_texture += std::max(0.0, terrain.animation_rate * delta);
		if (terrain.current_texture >= terrain.water_textures_nr) {
			terrain.current_texture = 0;
		}
	}

	// Map mouse coordinates to world coordinates
	if (input_handler.mouse != input_handler.previous_mouse) {
		glm::vec3 window = { input_handler.mouse.x, height - input_handler.mouse.y, 1.f };
		glm::vec3 pos = glm::unProject(window, camera->view, camera->projection, glm::vec4(0, 0, width, height));
		glm::vec3 origin = camera->position - camera->direction * camera->distance;
		glm::vec3 direction = glm::normalize(pos - origin);
		glm::vec3 toto = origin + direction * 2000.f;

		btVector3 from(origin.x, origin.y, origin.z);
		btVector3 to(toto.x, toto.y, toto.z);

		btCollisionWorld::ClosestRayResultCallback res(from, to);
		res.m_collisionFilterGroup = 32;
		res.m_collisionFilterMask = 32;
		physics.dynamicsWorld->rayTest(from, to, res);

		if (res.hasHit()) {
			auto& hit = res.m_hitPointWorld;
			input_handler.mouse_world = glm::vec3(hit.x(), hit.y(), hit.z());
		}
	}
}

void Map::render() {
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

	render_manager.render(render_lighting);

	//physics.dynamicsWorld->debugDrawWorld();
	//physics.draw->render();
}