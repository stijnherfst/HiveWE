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
	units_slk = slk::SLK2("Units/UnitData.slk");
	// By making some changes to unitmetadata.slk and unitdata.slk we can avoid the 1->2->2 mapping for SLK->OE->W3U files. We have to add some columns for this though
	units_slk.add_column("missilearc2");
	units_slk.add_column("missileart2");
	units_slk.add_column("missilespeed2");
	units_slk.add_column("buttonpos2");

	units_meta_slk = slk::SLK2("Data/Warcraft/UnitMetaData.slk", true);
	units_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	unit_editor_data = ini::INI("UI/UnitEditorData.txt");
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");
	// Have to substitute twice since some of the keys refer to other keys in the same file
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");


	units_slk.merge(ini::INI("Units/UnitSkin.txt"));
	units_slk.merge(ini::INI("Units/UnitWeaponsFunc.txt"));
	units_slk.merge(ini::INI("Units/UnitWeaponsSkin.txt"));
	
	units_slk.merge(slk::SLK2("Units/UnitBalance.slk"));
	units_slk.merge(slk::SLK2("Units/unitUI.slk"));
	units_slk.merge(slk::SLK2("Units/UnitWeapons.slk"));
	units_slk.merge(slk::SLK2("Units/UnitAbilities.slk"));

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

	abilities_slk = slk::SLK2("Units/AbilityData.slk");
	abilities_meta_slk = slk::SLK2("Units/AbilityMetaData.slk");
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
	items_slk = slk::SLK2("Units/ItemData.slk");
	items_slk.merge(ini::INI("Units/ItemSkin.txt"));
	items_slk.merge(ini::INI("Units/ItemFunc.txt"));
	items_slk.merge(ini::INI("Units/ItemStrings.txt"));

	items_meta_slk = slk::SLK2("Data/Warcraft/ItemMetaData.slk", true);
	items_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	// Doodads
	doodads_slk = slk::SLK2("Doodads/Doodads.slk");
	doodads_meta_slk = slk::SLK2("Doodads/DoodadMetaData.slk");
	doodads_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	doodads_slk.merge(ini::INI("Doodads/DoodadSkins.txt"));
	doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	// Destructables
	destructables_slk = slk::SLK2("Units/DestructableData.slk");
	destructables_meta_slk = slk::SLK2("Units/DestructableMetaData.slk");

	destructables_slk.merge(ini::INI("Units/DestructableSkin.txt"));
	destructables_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructables_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	upgrade_slk = slk::SLK2("Units/UpgradeData.slk");
	upgrade_meta_slk = slk::SLK2("Units/UpgradeMetaData.slk");
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

	buff_slk = slk::SLK2("Units/AbilityBuffData.slk");
	buff_meta_slk = slk::SLK2("Units/AbilityBuffMetaData.slk");
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

	std::cout << "SLK loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Trigger strings
	if (hierarchy.map_file_exists("war3map.wts")) {
		trigger_strings.load();
	}

	// Triggers (GUI and JASS)
	if (hierarchy.map_file_exists("war3map.wtg")) {
		triggers.load();

		// Custom text triggers (JASS)
		if (hierarchy.map_file_exists("war3map.wct")) {
			triggers.load_jass();
		}
	}

	std::cout << "Trigger loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	info.load();

	// Terrain
	terrain.load();

	std::cout << "Terrain loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Pathing Map
	pathing_map.load();

	std::cout << "Pathing loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Doodads
	doodads.load();

	if (hierarchy.map_file_exists("war3map.w3d")) {
		load_modification_file("war3map.w3d", doodads_slk, doodads_meta_slk, true);
	}

	if (hierarchy.map_file_exists("war3map.w3b")) {
		load_modification_file("war3map.w3b", destructables_slk, destructables_meta_slk, false);
	}

	doodads.create();

	std::cout << "Doodad loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	for (const auto& i : doodads.doodads) {
		if (!i.pathing) {
			continue;
		}

		if (doodads_slk.row_headers.contains(i.id)) {
			continue;
		}

		pathing_map.blit_pathing_texture(i.position, glm::degrees(i.angle) + 90, i.pathing);
	}
	pathing_map.upload_dynamic_pathing();

	std::cout << "Doodad blitting: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Units/Items
	if (hierarchy.map_file_exists("war3map.w3u")) {
		load_modification_file("war3map.w3u", units_slk, units_meta_slk, false);
	}

	if (hierarchy.map_file_exists("war3map.w3t")) {
		load_modification_file("war3map.w3t", items_slk, items_meta_slk, false);
	}

	if (hierarchy.map_file_exists("war3mapUnits.doo")) {
		units_loaded = units.load();

		if (units_loaded) {
			units.create();
		}
	}

	std::cout << "Unit loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Abilities 
	if (hierarchy.map_file_exists("war3map.w3a")) {
		load_modification_file("war3map.w3a", abilities_slk, abilities_meta_slk, true);
	}

	// Buffs
	if (hierarchy.map_file_exists("war3map.w3h")) {
		load_modification_file("war3map.w3h", buff_slk, buff_meta_slk, false);
	}

	// Upgrades
	if (hierarchy.map_file_exists("war3map.w3q")) {
		load_modification_file("war3map.w3q", upgrade_slk, upgrade_meta_slk, true);
	}

	// Regions
	if (hierarchy.map_file_exists("war3map.w3r")) {
		regions.load();
	}

	// Cameras
	if (hierarchy.map_file_exists("war3map.w3c")) {
		cameras.load();
	}

	// Sounds
	if (hierarchy.map_file_exists("war3map.w3s")) {
		sounds.load();
	}

	std::cout << "Regions/cameras/sounds loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

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

	save_modification_file("warmap.w3d", doodads_slk, doodads_meta_slk, true);
	save_modification_file("warmap.w3b", destructables_slk, destructables_meta_slk, false);
	doodads.save();

	save_modification_file("warmap.w3u", units_slk, units_meta_slk, false);
	save_modification_file("warmap.w3t", items_slk, items_meta_slk, false);
	units.save();

	// Currently mod file saving does not allow for "column offsets" used by abilities
	//save_modification_file("war3map.w3a", abilities_slk, abilities_meta_slk, true);

	save_modification_file("war3map.w3h", buff_slk, buff_meta_slk, false);
	save_modification_file("war3map.w3q", upgrade_slk, upgrade_meta_slk, true);

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
	terrain.render_ground();
	
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

	terrain.render_water();

	//physics.dynamicsWorld->debugDrawWorld();
	//physics.draw->render();
}