#include "Map.h"

#include <iostream>
#include <execution>
#include <random>

#include <QMessageBox>
#include <QOpenGLFunctions_4_5_Core>

#include "Hierarchy.h"
#include "HiveWE.h"
#include "InputHandler.h"
#include "Physics.h"
#include "Camera.h"

#include <fstream>
#include <bullet/btBulletDynamicsCommon.h>
#include <fmt/format.h>

using namespace std::literals::string_literals;

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
	// By making some changes to unitmetadata.slk and unitdata.slk we can avoid the 1->2->2 mapping for SLK->OE->W3U files. We have to add some columns for this though
	units_slk.add_column("missilearc2");
	units_slk.add_column("missileart2");
	units_slk.add_column("missilespeed2");
	units_slk.add_column("buttonpos2");

	units_meta_slk = slk::SLK("Data/Warcraft/UnitMetaData.slk", true);
	units_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	units_meta_slk.build_meta_map();

	unit_editor_data = ini::INI("UI/UnitEditorData.txt");
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");
	// Have to substitute twice since some of the keys refer to other keys in the same file
	unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");


	units_slk.merge(ini::INI("Units/UnitSkin.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/UnitWeaponsFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/UnitWeaponsSkin.txt"), units_meta_slk);
	
	units_slk.merge(slk::SLK("Units/UnitBalance.slk"));
	units_slk.merge(slk::SLK("Units/unitUI.slk"));
	units_slk.merge(slk::SLK("Units/UnitWeapons.slk"));
	units_slk.merge(slk::SLK("Units/UnitAbilities.slk"));

	units_slk.merge(ini::INI("Units/HumanUnitFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/OrcUnitFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/UndeadUnitFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/NightElfUnitFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/NeutralUnitFunc.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/CampaignUnitFunc.txt"), units_meta_slk);

	units_slk.merge(ini::INI("Units/HumanUnitStrings.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/OrcUnitStrings.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/UndeadUnitStrings.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/NightElfUnitStrings.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/NeutralUnitStrings.txt"), units_meta_slk);
	units_slk.merge(ini::INI("Units/CampaignUnitStrings.txt"), units_meta_slk);

	abilities_slk = slk::SLK("Units/AbilityData.slk");
	abilities_meta_slk = slk::SLK("Units/AbilityMetaData.slk");
	abilities_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	// Patch the SLKs
	abilities_slk.add_column("buttonpos2");
	abilities_slk.add_column("unbuttonpos2");
	abilities_slk.add_column("researchbuttonpos2");
	abilities_meta_slk.set_shadow_data("field", "abpy", "buttonpos2");
	abilities_meta_slk.set_shadow_data("field", "auby", "unbuttonpos2");
	abilities_meta_slk.set_shadow_data("field", "arpy", "researchbuttonpos2");
	abilities_meta_slk.build_meta_map();

	abilities_slk.merge(ini::INI("Units/AbilitySkin.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/AbilitySkinStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/HumanAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/OrcAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/UndeadAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/NightElfAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/NeutralAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/ItemAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/CommonAbilityFunc.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/CampaignAbilityFunc.txt"), abilities_meta_slk);

	abilities_slk.merge(ini::INI("Units/HumanAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/OrcAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/UndeadAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/NightElfAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/NeutralAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/ItemAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/CommonAbilityStrings.txt"), abilities_meta_slk);
	abilities_slk.merge(ini::INI("Units/CampaignAbilityStrings.txt"), abilities_meta_slk);

	// Items
	items_slk = slk::SLK("Units/ItemData.slk");
	items_meta_slk = slk::SLK("Data/Warcraft/ItemMetaData.slk", true);
	items_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	items_meta_slk.build_meta_map();

	items_slk.merge(ini::INI("Units/ItemSkin.txt"), items_meta_slk);
	items_slk.merge(ini::INI("Units/ItemFunc.txt"), items_meta_slk);
	items_slk.merge(ini::INI("Units/ItemStrings.txt"), items_meta_slk);


	// Doodads
	doodads_slk = slk::SLK("Doodads/Doodads.slk");
	doodads_meta_slk = slk::SLK("Doodads/DoodadMetaData.slk");
	doodads_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_meta_slk.build_meta_map();

	doodads_slk.merge(ini::INI("Doodads/DoodadSkins.txt"), doodads_meta_slk);
	doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
	doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	// Destructables
	destructibles_slk = slk::SLK("Units/DestructableData.slk");
	destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");

	destructibles_meta_slk = slk::SLK("Units/DestructableMetaData.slk");
	destructibles_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructibles_meta_slk.build_meta_map();

	//Fix Scorched tree
	destructibles_slk.merge(ini::INI("Data/Warcraft/DestructableSkin.txt", true), destructibles_meta_slk);

	destructibles_slk.merge(ini::INI("Units/DestructableSkin.txt"), destructibles_meta_slk);
	destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");
	destructibles_slk.substitute(world_edit_game_strings, "WorldEditStrings");

	upgrade_slk = slk::SLK("Units/UpgradeData.slk");
	upgrade_meta_slk = slk::SLK("Units/UpgradeMetaData.slk");
	upgrade_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

	// Patch the SLKs
	upgrade_slk.add_column("buttonpos2");
	upgrade_meta_slk.set_shadow_data("field", "gbpy", "buttonpos2");
	upgrade_meta_slk.build_meta_map();

	upgrade_slk.merge(ini::INI("Units/AbilitySkin.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/UpgradeSkin.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/HumanUpgradeFunc.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/OrcUpgradeFunc.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/UndeadUpgradeFunc.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/NightElfUpgradeFunc.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/NeutralUpgradeFunc.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeFunc.txt"), upgrade_meta_slk);

	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/HumanUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/NeutralUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/NightElfUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/OrcUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/UndeadUpgradeStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/UpgradeSkinStrings.txt"), upgrade_meta_slk);
	upgrade_slk.merge(ini::INI("Units/CampaignUpgradeFunc.txt"), upgrade_meta_slk);

	buff_slk = slk::SLK("Units/AbilityBuffData.slk");
	buff_meta_slk = slk::SLK("Units/AbilityBuffMetaData.slk");
	buff_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
	buff_meta_slk.build_meta_map();

	buff_slk.merge(ini::INI("Units/AbilitySkin.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/AbilitySkinStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/HumanAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/OrcAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/UndeadAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/NightElfAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/NeutralAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/ItemAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/CommonAbilityFunc.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/CampaignAbilityFunc.txt"), buff_meta_slk);
	
	buff_slk.merge(ini::INI("Units/HumanAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/OrcAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/UndeadAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/NightElfAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/NeutralAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/ItemAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/CommonAbilityStrings.txt"), buff_meta_slk);
	buff_slk.merge(ini::INI("Units/CampaignAbilityStrings.txt"), buff_meta_slk);

	units_table = new TableModel(&units_slk, &units_meta_slk);
	items_table = new TableModel(&items_slk, &items_meta_slk);
	abilities_table = new TableModel(&abilities_slk, &abilities_meta_slk);
	doodads_table = new TableModel(&doodads_slk, &doodads_meta_slk);
	destructibles_table = new TableModel(&destructibles_slk, &destructibles_meta_slk);
	upgrade_table = new TableModel(&upgrade_slk, &upgrade_meta_slk);
	buff_table = new TableModel(&buff_slk, &buff_meta_slk);

	std::cout << "\nSLK loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
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
	if (hierarchy.map_file_exists("war3map.wpm")) {
		pathing_map.load();
	}

	std::cout << "Pathing loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();

	// Doodads
	if (hierarchy.map_file_exists("war3map.w3d")) {
		load_modification_file("war3map.w3d", doodads_slk, doodads_meta_slk, true);
	}

	if (hierarchy.map_file_exists("war3map.w3b")) {
		load_modification_file("war3map.w3b", destructibles_slk, destructibles_meta_slk, false);
	}

	doodads.load();
	doodads.create();

	std::cout << "Doodad loading: " << (std::chrono::steady_clock::now() - begin).count() / 1'000'000 << "ms\n";
	begin = std::chrono::steady_clock::now();


	if (hierarchy.map_file_exists("war3map.w3u")) {
		load_modification_file("war3map.w3u", units_slk, units_meta_slk, false);
	}

	if (hierarchy.map_file_exists("war3map.w3t")) {
		load_modification_file("war3map.w3t", items_slk, items_meta_slk, false);
	}

	// Units/Items
	if (hierarchy.map_file_exists("war3mapUnits.doo")) {
		units.load();
		units.create();
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

	// Center camera
	camera->position = glm::vec3(terrain.width / 2, terrain.height / 2, 0);
	camera->position.z = terrain.interpolated_height(camera->position.x, camera->position.y);

	loaded = true;

	connect(units_table, &TableModel::dataChanged, [&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
		const std::string& id = units_slk.index_to_row.at(top_left.row());
		const std::string& field = units_slk.index_to_column.at(top_left.column());
		units.process_field_change(id, field);
	});

	connect(doodads_table, &TableModel::dataChanged, [&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
		const std::string& id = doodads_slk.index_to_row.at(top_left.row());
		const std::string& field = doodads_slk.index_to_column.at(top_left.column());
		doodads.process_doodad_field_change(id, field);
	});

	connect(destructibles_table, &TableModel::dataChanged, [&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
		const std::string& id = destructibles_slk.index_to_row.at(top_left.row());
		const std::string& field = destructibles_slk.index_to_column.at(top_left.column());
		doodads.process_destructible_field_change(id, field);
	});
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

	save_modification_file("war3map.w3d", doodads_slk, doodads_meta_slk, true);
	save_modification_file("war3map.w3b", destructibles_slk, destructibles_meta_slk, false);
	doodads.save();

	save_modification_file("war3map.w3u", units_slk, units_meta_slk, false);
	save_modification_file("war3map.w3t", items_slk, items_meta_slk, false);
	units.save();

	save_modification_file("war3map.w3a", abilities_slk, abilities_meta_slk, true);

	save_modification_file("war3map.w3h", buff_slk, buff_meta_slk, false);
	save_modification_file("war3map.w3q", upgrade_slk, upgrade_meta_slk, true);

	info.save();
	trigger_strings.save();
	triggers.save();
	triggers.save_jass();
	triggers.generate_map_script();
	imports.save();

	return true;
}

void Map::update(double delta, int width, int height) {
	if (!loaded) {
		return;
	}

	camera->update(delta);

	// Update current water texture index
	terrain.current_texture += std::max(0.0, terrain.animation_rate * delta);
	if (terrain.current_texture >= terrain.water_textures_nr) {
		terrain.current_texture = 0;
	}

	/*auto current_time = std::chrono::steady_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(current_time).count() / 1000.f;
	light_direction = glm::normalize(glm::vec3(std::cos(seconds), std::sin(seconds), -2.f));*/

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
			input_handler.previous_mouse_world = input_handler.mouse_world;
			input_handler.mouse_world = glm::vec3(hit.x(), hit.y(), hit.z());
		}
	}

	// Animate units
	std::for_each(std::execution::par_unseq, units.units.begin(), units.units.end(), [&](Unit& i) {
		if (i.id == "sloc") {
			return;
		} // ToDo handle starting locations

		mdx::Extent& extent = i.mesh->model->sequences[i.skeleton.sequence_index].extent;
		if (!camera->inside_frustrum(i.matrix * glm::vec4(extent.minimum, 1.f), i.matrix * glm::vec4(extent.maximum, 1.f))) {
			return;
		}

		i.skeleton.update(delta);
	});

	// Animate items
	for (auto& i : units.items) {
		i.skeleton.update(delta);
	}
}

void Map::render() {
	// While switching maps it may happen that render is called before loading has finished.
	if (!loaded) {
		return;
	}

	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl->glPolygonMode(GL_FRONT_AND_BACK, render_wireframe ? GL_LINE : GL_FILL);

	terrain.render_ground();

	if (render_doodads) {
		doodads.render();
	}

	if (render_units) {
		units.render();
	}

	if (render_brush && brush) {
		brush->render();
	}

	render_manager.render(render_lighting, light_direction);

	terrain.render_water();

	//physics.dynamicsWorld->debugDrawWorld();
	//physics.draw->render();
}

void Map::resize(size_t width, size_t height) {
	terrain.resize(width, height);
	pathing_map.resize(width * 4, height * 4);
}

std::string Map::get_unique_id(bool first_uppercase) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 25);
again:

	std::string id = ""s + char((first_uppercase ? 'A' : 'a') + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt));

	if (units_slk.row_headers.contains(id) 
		|| items_slk.row_headers.contains(id) 
		|| abilities_slk.row_headers.contains(id)
		|| doodads_slk.row_headers.contains(id) 
		|| destructibles_slk.row_headers.contains(id)
		|| upgrade_slk.row_headers.contains(id)
		|| buff_slk.row_headers.contains(id)) {
		
		std::cout << "Generated an existing ID: " << id << " what're the odds\n";
		goto again;
	}

	return id;
}