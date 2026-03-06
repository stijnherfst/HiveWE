module;

#include <QMessageBox>

export module Map;

import std;
import types;
import MDX;
import SLK;
import INI;
import GameCameras;
import Imports;
import MapInfo;
import Doodad;
import Sounds;
import Regions;
import WorldUndoManager;
import TriggerStrings;
import PathingMap;
import ShadowMap;
import Physics;
import Hierarchy;
import Camera;
import Timer;
import Physics;
import ModificationTables;
import RenderManager;
import TableModel;
import Globals;
import Units;
import Doodads;
import Triggers;
import Terrain;
import GameplayConstants;
import "brush.h";
import <glad/glad.h>;
import <bullet/btBulletDynamicsCommon.h>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

namespace fs = std::filesystem;
using namespace std::literals::string_literals;

export class Map: public QObject {
	Q_OBJECT

  public:
	bool loaded = false;

	TriggerStrings trigger_strings;
	Triggers triggers;
	MapInfo info;
	Terrain terrain;
	PathingMap pathing_map;
	Imports imports;
	Doodads doodads;
	Units units;
	Regions regions;
	GameCameras cameras;
	Sounds sounds;
	GameplayConstants gameplay_constants;
	ShadowMap shadow_map;
	WorldUndoManager world_undo;
	Brush* brush = nullptr;
	Physics physics;

	bool enforce_water_height_limits = true;

	bool render_doodads = true;
	bool render_units = true;
	bool render_pathing = false;
	bool render_brush = true;
	bool render_lighting = true;
	bool render_water = true;
	bool render_click_helpers = true;
	bool render_wireframe = false;
	bool render_debug = false;

	glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 1.f, -3.f));

	fs::path filesystem_path;
	std::string name;

	RenderManager render_manager;

	void load(const fs::path& path) {
		Timer timer;

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

		units_meta_slk = slk::SLK("Units/UnitMetaData.slk");
		units_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
		units_meta_slk.build_meta_map();

		unit_editor_data = ini::INI("UI/UnitEditorData.txt");
		unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");
		// Have to substitute twice since some keys refer to other keys in the same file
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
		items_meta_slk = slk::SLK("Units/ItemMetaData.slk");
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

		// Sometimes fields are empty or "-" which denotes empty aka the value 0.0
		for (auto& [key, fields] : doodads_slk.base_data) {
			if (auto found = fields.find("maxpitch"); found != fields.end()) {
				if (found->second.empty() || found->second == "-") {
					found->second = "0";
				}
			} else {
				fields["maxpitch"] = "0";
			}
			if (auto found = fields.find("maxroll"); found != fields.end()) {
				if (found->second.empty() || found->second == "-") {
					found->second = "0";
				}
			} else {
				fields["maxroll"] = "0";
			}
		}

		// Destructables
		destructibles_slk = slk::SLK("Units/DestructableData.slk");
		destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");

		destructibles_meta_slk = slk::SLK("Units/DestructableMetaData.slk");
		destructibles_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
		destructibles_meta_slk.build_meta_map();

		destructibles_slk.merge(ini::INI("Units/DestructableSkin.txt"), destructibles_meta_slk);
		destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");
		destructibles_slk.substitute(world_edit_game_strings, "WorldEditStrings");

		// Sometimes fields are empty or "-" which denotes empty aka the value 0.0
		for (auto& [key, fields] : destructibles_slk.base_data) {
			if (auto found = fields.find("maxpitch"); found != fields.end()) {
				if (found->second.empty() || found->second == "-") {
					found->second = "0";
				}
			} else {
				fields["maxpitch"] = "0";
			}
			if (auto found = fields.find("maxroll"); found != fields.end()) {
				if (found->second.empty() || found->second == "-") {
					found->second = "0";
				}
			} else {
				fields["maxroll"] = "0";
			}
		}

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

		units_table = new TableModel(&units_slk, &units_meta_slk, &trigger_strings);
		items_table = new TableModel(&items_slk, &items_meta_slk, &trigger_strings);
		abilities_table = new TableModel(&abilities_slk, &abilities_meta_slk, &trigger_strings);
		doodads_table = new TableModel(&doodads_slk, &doodads_meta_slk, &trigger_strings);
		destructibles_table = new TableModel(&destructibles_slk, &destructibles_meta_slk, &trigger_strings);
		upgrade_table = new TableModel(&upgrade_slk, &upgrade_meta_slk, &trigger_strings);
		buff_table = new TableModel(&buff_slk, &buff_meta_slk, &trigger_strings);

		std::println("\nSLK loading:\t {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Trigger strings
		if (hierarchy.map_file_exists("war3map.wts")) {
			trigger_strings.load();
		}

		// Triggers (GUI and JASS)
		if (hierarchy.map_file_exists("war3map.wtg")) {
			triggers.load();

			// Custom text triggers (JASS)
			if (hierarchy.map_file_exists("war3map.wct")) {
				triggers.load_scripts();
			}
		}

		std::println("Trigger loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		gameplay_constants.load();

		info.load();
		terrain.load(physics);

		std::println("Terrain loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Pathing Map
		if (hierarchy.map_file_exists("war3map.wpm")) {
			pathing_map.load(terrain.width, terrain.height);
		} else {
			pathing_map.resize(terrain.width * 4, terrain.height * 4);
		}

		std::println("Pathing loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Doodads
		if (hierarchy.map_file_exists("war3map.w3d")) {
			load_modification_file("war3map.w3d", doodads_slk, doodads_meta_slk, true);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3d")) {
			load_modification_file("war3mapSkin.w3d", doodads_slk, doodads_meta_slk, true);
		}

		if (hierarchy.map_file_exists("war3map.w3b")) {
			load_modification_file("war3map.w3b", destructibles_slk, destructibles_meta_slk, false);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3b")) {
			load_modification_file("war3mapSkin.w3b", destructibles_slk, destructibles_meta_slk, false);
		}

		doodads.load(terrain, info);
		doodads.create(terrain, pathing_map);

		std::println("Doodad loading:\t {:>5}ms", timer.elapsed_ms());
		timer.reset();

		if (hierarchy.map_file_exists("war3map.w3u")) {
			load_modification_file("war3map.w3u", units_slk, units_meta_slk, false);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3u")) {
			load_modification_file("war3mapSkin.w3u", units_slk, units_meta_slk, false);
		}

		if (hierarchy.map_file_exists("war3map.w3t")) {
			load_modification_file("war3map.w3t", items_slk, items_meta_slk, false);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3t")) {
			load_modification_file("war3mapSkin.w3t", items_slk, items_meta_slk, false);
		}

		// Units/Items
		if (hierarchy.map_file_exists("war3mapUnits.doo")) {
			units.load(terrain, info);
			units.create();
		}

		std::println("Unit loading:\t {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Abilities
		if (hierarchy.map_file_exists("war3map.w3a")) {
			load_modification_file("war3map.w3a", abilities_slk, abilities_meta_slk, true);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3a")) {
			load_modification_file("war3mapSkin.w3a", abilities_slk, abilities_meta_slk, true);
		}

		// Buffs
		if (hierarchy.map_file_exists("war3map.w3h")) {
			load_modification_file("war3map.w3h", buff_slk, buff_meta_slk, false);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3h")) {
			load_modification_file("war3mapSkin.w3h", buff_slk, buff_meta_slk, false);
		}

		// Upgrades
		if (hierarchy.map_file_exists("war3map.w3q")) {
			load_modification_file("war3map.w3q", upgrade_slk, upgrade_meta_slk, true);
		}

		if (hierarchy.map_file_exists("war3mapSkin.w3q")) {
			load_modification_file("war3mapSkin.w3q", upgrade_slk, upgrade_meta_slk, true);
		}

		// Regions
		if (hierarchy.map_file_exists("war3map.w3r")) {
			regions.load();
		}

		// Cameras
		if (hierarchy.map_file_exists("war3map.w3c")) {
			cameras.load(info.game_version_major, info.game_version_minor);
		}

		// Sounds
		if (hierarchy.map_file_exists("war3map.w3s")) {
			sounds.load();
		}

		std::println("Misc loading:\t {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Shadow map
		if (hierarchy.map_file_exists("war3map.shd")) {
			shadow_map.load(terrain.width - 1, terrain.height - 1);
		} else {
			shadow_map.resize((terrain.width - 1) * 4, (terrain.height - 1) * 4);
		}

		std::println("Shadows loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Center camera
		camera.position = glm::vec3(terrain.width / 2, terrain.height / 2, 0);
		camera.position.z = terrain.interpolated_height(camera.position.x, camera.position.y, true);

		loaded = true;

		connect(
			units_table,
			&TableModel::dataChanged,
			[&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
				const std::string& id = units_slk.index_to_row.at(top_left.row());
				const std::string& field = units_slk.index_to_column.at(top_left.column());
				units.process_unit_field_change(id, field);
			}
		);

		connect(units_table, &TableModel::rowsAboutToBeRemoved, [&](const QModelIndex& parent, int first, int last) {
			for (size_t i = first; i <= last; i++) {
				const std::string& id = units_slk.index_to_row.at(i);
				std::erase_if(units.units, [&](Unit& unit) {
					return unit.id == id;
				});

				if (brush) {
					brush->unselect_id(id);
				}
			}
		});

		connect(
			items_table,
			&TableModel::dataChanged,
			[&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
				const std::string& id = items_slk.index_to_row.at(top_left.row());
				const std::string& field = items_slk.index_to_column.at(top_left.column());
				units.process_item_field_change(id, field);
			}
		);

		connect(items_table, &TableModel::rowsAboutToBeRemoved, [&](const QModelIndex& parent, int first, int last) {
			for (size_t i = first; i <= last; i++) {
				const std::string& id = items_slk.index_to_row.at(i);
				std::erase_if(units.items, [&](Unit& item) {
					return item.id == id;
				});
			}
		});

		connect(
			doodads_table,
			&TableModel::dataChanged,
			[&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
				const std::string& id = doodads_slk.index_to_row.at(top_left.row());
				const std::string& field = doodads_slk.index_to_column.at(top_left.column());
				doodads.process_doodad_field_change(id, field, terrain);
			}
		);

		connect(doodads_table, &TableModel::rowsAboutToBeRemoved, [&](const QModelIndex& parent, int first, int last) {
			for (size_t i = first; i <= last; i++) {
				const std::string& id = doodads_slk.index_to_row.at(i);
				std::erase_if(doodads.doodads, [&](Doodad& doodad) {
					return doodad.id == id;
				});

				if (brush) {
					brush->unselect_id(id);
				}
			}
		});

		connect(
			destructibles_table,
			&TableModel::dataChanged,
			[&](const QModelIndex& top_left, const QModelIndex& top_right, const QVector<int>& roles) {
				const std::string& id = destructibles_slk.index_to_row.at(top_left.row());
				const std::string& field = destructibles_slk.index_to_column.at(top_left.column());
				doodads.process_destructible_field_change(id, field, terrain);
			}
		);

		connect(destructibles_table, &TableModel::rowsAboutToBeRemoved, [&](const QModelIndex& parent, int first, int last) {
			for (size_t i = first; i <= last; i++) {
				const std::string& id = destructibles_slk.index_to_row.at(i);
				std::erase_if(doodads.doodads, [&](Doodad& destructable) {
					return destructable.id == id;
				});

				if (brush) {
					brush->unselect_id(id);
				}
			}
		});
	}

	bool save(const fs::path& path) {
		Timer timer;
		if (!fs::equivalent(path, filesystem_path)) {
			try {
				fs::copy(filesystem_path, fs::absolute(path), fs::copy_options::recursive);
			} catch (fs::filesystem_error& e) {
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
		shadow_map.save();

		save_modification_file("war3map.w3d", doodads_slk, doodads_meta_slk, true, false);
		save_modification_file("war3mapSkin.w3d", doodads_slk, doodads_meta_slk, true, true);
		save_modification_file("war3map.w3b", destructibles_slk, destructibles_meta_slk, false, false);
		save_modification_file("war3mapSkin.w3b", destructibles_slk, destructibles_meta_slk, false, true);
		doodads.save(terrain);

		save_modification_file("war3map.w3u", units_slk, units_meta_slk, false, false);
		save_modification_file("war3mapSkin.w3u", units_slk, units_meta_slk, false, true);
		save_modification_file("war3map.w3t", items_slk, items_meta_slk, false, false);
		save_modification_file("war3mapSkin.w3t", items_slk, items_meta_slk, false, true);
		units.save(terrain);

		save_modification_file("war3map.w3a", abilities_slk, abilities_meta_slk, true, false);
		save_modification_file("war3mapSkin.w3a", abilities_slk, abilities_meta_slk, true, true);

		save_modification_file("war3map.w3h", buff_slk, buff_meta_slk, false, false);
		save_modification_file("war3mapSkin.w3h", buff_slk, buff_meta_slk, false, true);
		save_modification_file("war3map.w3q", upgrade_slk, upgrade_meta_slk, true, false);
		save_modification_file("war3mapSkin.w3q", upgrade_slk, upgrade_meta_slk, true, true);

		info.save(terrain.tileset);
		trigger_strings.save();
		triggers.save();
		triggers.save_scripts();
		ScriptMode mode = ScriptMode::jass;
		if (info.lua) {
			mode = ScriptMode::lua;
		}

		const auto result = triggers.generate_map_script(terrain, units, doodads, info, sounds, regions, cameras, mode);
		if (!result.has_value()) {
			QMessageBox::information(
				nullptr,
				"vJass output",
				"There were compilation errors:\n" + QString::fromStdString(result.error()),
				QMessageBox::StandardButton::Ok
			);
		}

		gameplay_constants.save();

		imports.save(filesystem_path);

		std::println("Saving took: {:>5}ms", timer.elapsed_ms());

		return true;
	}

	void update(double delta, int width, int height) {
		if (!loaded) {
			return;
		}

		camera.position.z = terrain.interpolated_height(camera.position.x, camera.position.y, true);
		camera.update(delta);

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
			glm::vec3 window = {input_handler.mouse.x, height - input_handler.mouse.y, 1.f};
			glm::vec3 pos = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, width, height));
			glm::vec3 origin = camera.position - camera.direction * camera.distance;
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

			mdx::Extent& extent = i.mesh->mdx->sequences[i.skeleton.sequence_index].extent;
			if (!camera.inside_frustrum_transform(extent.minimum, extent.maximum, i.skeleton.matrix)) {
				return;
			}

			i.skeleton.update(delta);
		});

		// Animate items
		for (auto& i : units.items) {
			i.skeleton.update(delta);
		}

		// Animate doodads
		std::for_each(std::execution::par_unseq, doodads.doodads.begin(), doodads.doodads.end(), [&](Doodad& i) {
			mdx::Extent& extent = i.mesh->mdx->sequences[i.skeleton.sequence_index].extent;
			if (!camera.inside_frustrum_transform(extent.minimum, extent.maximum, i.skeleton.matrix)) {
				return;
			}

			i.skeleton.update(delta);
		});
	}

	void render() {
		// While switching maps it may happen that render is called before loading has finished.
		if (!loaded) {
			return;
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, render_wireframe ? GL_LINE : GL_FILL);

		terrain.render_ground(render_pathing, render_lighting, light_direction, brush, pathing_map);

		if (render_doodads) {
			for (const auto& i : doodads.doodads) {
				render_manager.queue_render(*i.mesh, i.skeleton, i.color, 0);
				if (render_click_helpers) {
					const bool is_doodad = doodads_slk.row_headers.contains(i.id);
					const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
					if (slk.data<bool>("useclickhelper", i.id)) {
						render_manager.queue_click_helper(i.skeleton.matrix);
					}
				}
			}
			for (const auto& i : doodads.special_doodads) {
				render_manager.queue_render(*i.mesh, i.skeleton, glm::vec3(1.f), 0);
			}
		}

		if (render_units) {
			for (auto& i : units.units) {
				if (i.id == "sloc") {
					continue;
				} // ToDo handle starting locations

				render_manager.queue_render(*i.mesh, i.skeleton, i.color, i.player);
			}
			for (auto& i : units.items) {
				render_manager.queue_render(*i.mesh, i.skeleton, i.color, i.player);
			}
		}

		if (render_brush && brush) {
			brush->render();
		}

		render_manager.render(render_lighting, render_click_helpers, light_direction);
		if (render_water) {
			terrain.render_water();
		}

		// physics.dynamicsWorld->debugDrawWorld();
		// physics.draw->render();
	}

	void resize(size_t width, size_t height) {
		terrain.resize(width, height);
		pathing_map.resize(width * 4, height * 4);
	}

	/// Resizes the entire map by expanding/shirnking it from all sides
	/// Handles terrain, pathing map, shadow map and preplaced objects
	/// Also, as per vanilla WE behaviour, clears the entire world undo stack
	void resize(int delta_left, int delta_right, int delta_top, int delta_bottom) {
		terrain.resize(delta_left, delta_right, delta_top, delta_bottom, physics);

		// update the pathing and shadow maps to the new size
		pathing_map.resize(delta_left * 4, delta_right * 4, delta_top * 4, delta_bottom * 4);
		shadow_map.resize(delta_left * 4, delta_right * 4, delta_top * 4, delta_bottom * 4);

		// update object positions
		update_object_positions(delta_left, delta_right, delta_top, delta_bottom);

		// vanilla WE behaviour: clear unit/doodad/terrain undo
		world_undo.clear_all_undo();

		// since this function does not change playable area, we have to update the map info accordingly
		info.camera_complements[0] += delta_left;
		info.camera_complements[1] += delta_right;
		info.camera_complements[2] += delta_bottom;
		info.camera_complements[3] += delta_top;

		// finally, move the camera
		camera.position.x += delta_left / 2;
		camera.position.y += delta_bottom / 2;
		camera.update(0.0);
	}

	/// Sets the playable area (shadowed map bounds)
	/// Handles the terrain flags, camera bounds and map info
	/// Also updates the pathing map and deletes units/items which are now out of bounds
	void set_playable_area(int unplayable_left, int unplayable_right, int unplayable_top, int unplayable_bottom) {
		int old_left = info.camera_complements[0];
		int old_right = info.camera_complements[1];
		int old_bottom = info.camera_complements[2];
		int old_top = info.camera_complements[3];

		// apply the shadowed camera boundaries on the edges
		terrain.set_unplayable_boundaries(unplayable_left, unplayable_right, unplayable_top, unplayable_bottom);

		// reset the pathing map - old unplayable segments should
		// no longer be unwalkable/unflyable/unbuildable
		reset_map_edge_pathing(
			old_left,
			old_right,
			old_top,
			old_bottom,
			unplayable_left,
			unplayable_right,
			unplayable_top,
			unplayable_bottom
		);

		// update unplayable area in map info
		info.camera_complements[0] = unplayable_left;
		info.camera_complements[1] = unplayable_right;
		info.camera_complements[2] = unplayable_bottom;
		info.camera_complements[3] = unplayable_top;

		// update playable map area
		info.playable_width = terrain.width - 1 - info.camera_complements[0] - info.camera_complements[1];
		info.playable_height = terrain.height - 1 - info.camera_complements[2] - info.camera_complements[3];

		// compute camera bounds based on complements and terrain offset
		// these bounds are used in the generated JASS script
		info.camera_left_bottom.x = (info.camera_complements[0] + 4) * 128.f + terrain.offset.x;
		info.camera_left_bottom.y = (info.camera_complements[2] + 2) * 128.f + terrain.offset.y;

		info.camera_right_top.x = (terrain.width - 1 - info.camera_complements[1] - 4) * 128.f + terrain.offset.x;
		info.camera_right_top.y = (terrain.height - 1 - info.camera_complements[3] - 2) * 128.f + terrain.offset.y;

		info.camera_left_top.x = (info.camera_complements[0] + 4) * 128.f + terrain.offset.x;
		info.camera_left_top.y = (terrain.height - 1 - info.camera_complements[3] - 2) * 128.f + terrain.offset.y;

		info.camera_right_bottom.x = (terrain.width - 1 - info.camera_complements[1] - 4) * 128.f + terrain.offset.x;
		info.camera_right_bottom.y = (info.camera_complements[2] + 2) * 128.f + terrain.offset.y;
	}

	std::string get_unique_id(bool first_uppercase) {
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(0, 25);
	again:

		std::string id =
			""s + char((first_uppercase ? 'A' : 'a') + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt));

		if (units_slk.row_headers.contains(id) || items_slk.row_headers.contains(id) || abilities_slk.row_headers.contains(id)
			|| doodads_slk.row_headers.contains(id) || destructibles_slk.row_headers.contains(id) || upgrade_slk.row_headers.contains(id)
			|| buff_slk.row_headers.contains(id)) {
			std::print("Generated an existing ID: {} what're the odds\n", id);
			goto again;
		}

		return id;
	}

  private:
	/// called when resizing the map
	/// in HiveWE objeccts positions are saved relative to the bottom-left corner of the terrain
	/// resizing the map may nudge all preplaced objeccts (units, doodads...)
	/// calling this function will restore original positions after the resize
	/// also, if will delete objects which are now out of bounds
	int update_object_positions(int delta_left, int delta_right, int delta_top, int delta_bottom) {
		size_t num_deleted = 0;

		// terrain is already resized here
		const int width = terrain.width - 1;
		const int height = terrain.height - 1;

		// helper to update positions and collect out-of-bounds objects for removal
		auto update_and_collect_unit = [&](auto& container) {
			std::unordered_set<Unit*> to_delete;

			for (auto& obj : container) {
				// update the object position relative to the change in bottom-left corner
				obj.position.x += delta_left;
				obj.position.y += delta_bottom;

				// check if the object is outside map bounds
				if (obj.position.x < 0 || obj.position.y < 0 || obj.position.x > width || obj.position.y > height) {
					to_delete.insert(&obj);
					++num_deleted;
				} else {
					// update rendered object position
					obj.update();
				}
			}

			return to_delete;
		};

		auto update_and_collect_doodad = [&](auto& container) {
			std::unordered_set<Doodad*> to_delete;

			for (auto& obj : container) {
				// update the object position relative to the change in bottom-left corner
				obj.position.x += delta_left;
				obj.position.y += delta_bottom;

				// check if the object is outside map bounds
				if (obj.position.x < 0 || obj.position.y < 0 || obj.position.x > width || obj.position.y > height) {
					to_delete.insert(&obj);
					++num_deleted;
				} else {
					// update rendered object position
					obj.update(terrain);
				}
			}

			return to_delete;
		};

		// fix/remove all objects
		units.remove_units(update_and_collect_unit(units.units));
		units.remove_items(update_and_collect_unit(units.items));
		doodads.remove_doodads(update_and_collect_doodad(doodads.doodads));

		// TODO: once special doodads are implemented, handle them as well
		//doodads.remove_special_doodads(update_and_collect_doodad(doodads.special_doodads));

		// TODO: once cameras and regions are implemented, handle them too

		return num_deleted;
	}

	/// Recomputes the terrain pathing on the edges of the map
	void reset_map_edge_pathing(
		int old_left,
		int old_right,
		int old_top,
		int old_bottom,
		int new_left,
		int new_right,
		int new_top,
		int new_bottom
	) {
		int width = (terrain.width - 1) * 4;
		int height = (terrain.height - 1) * 4;

		// old unplayable segments (on the pathing map)
		int old_left_p = old_left * 4;
		int old_right_p = old_right * 4;
		int old_bottom_p = old_bottom * 4;
		int old_top_p = old_top * 4;

		// new unplayable segments (on the pathing map)
		int new_left_p = new_left * 4;
		int new_right_p = new_right * 4;
		int new_bottom_p = new_bottom * 4;
		int new_top_p = new_top * 4;

		constexpr uint8_t edge_pathing = PathingMap::unwalkable | PathingMap::unflyable | PathingMap::unbuildable;

		for (size_t i = 0; i < width; ++i) {
			for (size_t j = 0; j < height; ++j) {
				// check if outside new boundaries (in unplayable area)
				bool outside_new = (i < new_left_p || i >= width - new_right_p || j < new_bottom_p || j >= height - new_top_p);

				// check if outside old boundaries (in old unplayable area)
				bool outside_old = (i < old_left_p || i >= width - old_right_p || j < old_bottom_p || j >= height - old_top_p);

				// update the pathing map
				if (outside_new) {
					pathing_map.pathing_cells_static[j * width + i] = edge_pathing;
				} else if (outside_old) {
					pathing_map.pathing_cells_static[j * width + i] = terrain.get_terrain_pathing(i, j);
				}
			}
		}

		pathing_map.upload_static_pathing();
	}
};

#include "map.moc"
