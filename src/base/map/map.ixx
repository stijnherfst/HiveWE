module;

#include <QMessageBox>
#include <volk.h>

export module Map;

import std;
import types;
import MDX;
import SLK;
import INI;
import BinaryReader;
import GameCameras;
import Imports;
import MapInfo;
import Tileset;
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
import Utilities;
import UnorderedMap;
import VkResources;
import VkBrushOverlay;
import "brush.h";
import "region_brush.h";
import <bullet/btBulletDynamicsCommon.h>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

namespace fs = std::filesystem;
using namespace std::literals::string_literals;

export struct FileUsage {
	std::string path; // original on-disk relative path (forward slashes, original case)
	u64 size = 0;
	std::unordered_set<std::string> used_by; // empty = unused
};

/// Starts reading and parsing the SLK or INI at `path` on another thread
template <typename T>
std::future<T> start_loading(const char* path) {
	return std::async(std::launch::async, [path] { return T(path); });
}

/// Merges the INIs into `slk` in order, each as soon as it has loaded
void merge_inis(slk::SLK& slk, const slk::SLK& meta_slk, auto& inis) {
	for (auto& ini : inis) {
		slk.merge(ini.get(), meta_slk);
	}
}

/// Sometimes fields are empty or "-" which denotes empty aka the value 0.0
void default_pitch_and_roll(slk::SLK& slk) {
	for (auto& [key, fields] : slk.base_data) {
		for (const char* field : {"maxpitch", "maxroll"}) {
			std::string& value = fields[field];
			if (value.empty() || value == "-") {
				value = "0";
			}
		}
	}
}

export class Map: public QObject {
	Q_OBJECT

  public:
	bool loaded = false;
	/// The map was newly created and has not been resaved yet
	bool is_in_temp_dir = false;

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
	TilesetData tilesets;
	WorldUndoManager world_undo;
	Brush* brush = nullptr;
	Physics physics;

	bool enforce_water_height_limits = true;

	bool render_doodads = true;
	bool render_units = true;
	bool render_pathing = false;
	bool render_regions = false;
	bool render_brush = true;
	bool render_lighting = true;
	bool render_water = true;
	bool render_click_helpers = true;
	bool render_wireframe = false;
	bool render_debug = false;

	/// Scratch for culling doodads in parallel
	std::vector<uint8_t> doodads_in_view;

	glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 1.f, -3.f));

	fs::path filesystem_path;
	std::string name;

	RenderManager render_manager;
	BrushOverlayRenderer brush_overlay;

	/// Bundles up the parts of the map that world edits act on, so that brushes and
	/// undo commands take what they need as a parameter instead of reaching for the
	/// map global.
	WorldEditContext edit_context() {
		return WorldEditContext {
			.terrain = terrain,
			.units = units,
			.doodads = doodads,
			.regions = regions,
			.brush = brush,
			.pathing_map = pathing_map,
		};
	}

	void load(const fs::path& path) {
		Timer full_timer;
		Timer timer;

		hierarchy.map_directory = path;
		filesystem_path = fs::absolute(path) / "";
		name = (--(--filesystem_path.end()))->string();

		// ToDo So for the game data files we should actually load from _balance/custom_v0.w3mod/Units, _balance/custom_v1.w3mod/Units, _balance/melee_v0.w3mod/units or /Units depending on the Game Data set and Game Data Versions
		// Maybe just ignore RoC so we only need to choose between _balance/custom_v1.w3mod/Units and /Units
		// Maybe just force everyone to suck it up and use /Units
		using ini::INI;
		using slk::SLK;

		// Merged into the abilities and the buffs, so they are loaded once up front
		const std::shared_future<INI> ability_skin = start_loading<INI>("Units/AbilitySkin.txt").share();
		const std::array<std::shared_future<INI>, 18> ability_inis = {
			ability_skin,
			start_loading<INI>("Units/AbilitySkinStrings.txt").share(),
			start_loading<INI>("Units/HumanAbilityFunc.txt").share(),
			start_loading<INI>("Units/OrcAbilityFunc.txt").share(),
			start_loading<INI>("Units/UndeadAbilityFunc.txt").share(),
			start_loading<INI>("Units/NightElfAbilityFunc.txt").share(),
			start_loading<INI>("Units/NeutralAbilityFunc.txt").share(),
			start_loading<INI>("Units/ItemAbilityFunc.txt").share(),
			start_loading<INI>("Units/CommonAbilityFunc.txt").share(),
			start_loading<INI>("Units/CampaignAbilityFunc.txt").share(),
			start_loading<INI>("Units/HumanAbilityStrings.txt").share(),
			start_loading<INI>("Units/OrcAbilityStrings.txt").share(),
			start_loading<INI>("Units/UndeadAbilityStrings.txt").share(),
			start_loading<INI>("Units/NightElfAbilityStrings.txt").share(),
			start_loading<INI>("Units/NeutralAbilityStrings.txt").share(),
			start_loading<INI>("Units/ItemAbilityStrings.txt").share(),
			start_loading<INI>("Units/CommonAbilityStrings.txt").share(),
			start_loading<INI>("Units/CampaignAbilityStrings.txt").share(),
		};

		auto units_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/UnitData.slk");
			auto meta = start_loading<SLK>("Units/UnitMetaData.slk");
			auto editor_data = start_loading<INI>("UI/UnitEditorData.txt");
			std::array<std::future<INI>, 3> skin_inis = {
				start_loading<INI>("Units/UnitSkin.txt"),
				start_loading<INI>("Units/UnitWeaponsFunc.txt"),
				start_loading<INI>("Units/UnitWeaponsSkin.txt"),
			};
			std::array<std::future<SLK>, 4> slks = {
				start_loading<SLK>("Units/UnitBalance.slk"),
				start_loading<SLK>("Units/unitUI.slk"),
				start_loading<SLK>("Units/UnitWeapons.slk"),
				start_loading<SLK>("Units/UnitAbilities.slk"),
			};
			std::array<std::future<INI>, 12> race_inis = {
				start_loading<INI>("Units/HumanUnitFunc.txt"),
				start_loading<INI>("Units/OrcUnitFunc.txt"),
				start_loading<INI>("Units/UndeadUnitFunc.txt"),
				start_loading<INI>("Units/NightElfUnitFunc.txt"),
				start_loading<INI>("Units/NeutralUnitFunc.txt"),
				start_loading<INI>("Units/CampaignUnitFunc.txt"),
				start_loading<INI>("Units/HumanUnitStrings.txt"),
				start_loading<INI>("Units/OrcUnitStrings.txt"),
				start_loading<INI>("Units/UndeadUnitStrings.txt"),
				start_loading<INI>("Units/NightElfUnitStrings.txt"),
				start_loading<INI>("Units/NeutralUnitStrings.txt"),
				start_loading<INI>("Units/CampaignUnitStrings.txt"),
			};

			units_slk = data.get();
			// By making some changes to unitmetadata.slk and unitdata.slk we can avoid the 1->2->2 mapping for SLK->OE->W3U files. We have to add some columns for this though
			units_slk.add_column("missilearc2");
			units_slk.add_column("missileart2");
			units_slk.add_column("missilespeed2");
			units_slk.add_column("buttonpos2");

			units_meta_slk = meta.get();
			units_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
			units_meta_slk.build_meta_map();

			unit_editor_data = editor_data.get();
			unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");
			// Have to substitute twice since some keys refer to other keys in the same file
			unit_editor_data.substitute(world_edit_strings, "WorldEditStrings");

			merge_inis(units_slk, units_meta_slk, skin_inis);
			for (auto& slk : slks) {
				units_slk.merge(slk.get());
			}
			merge_inis(units_slk, units_meta_slk, race_inis);
		});

		auto items_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/ItemData.slk");
			auto meta = start_loading<SLK>("Units/ItemMetaData.slk");
			std::array<std::future<INI>, 3> inis = {
				start_loading<INI>("Units/ItemSkin.txt"),
				start_loading<INI>("Units/ItemFunc.txt"),
				start_loading<INI>("Units/ItemStrings.txt"),
			};

			items_slk = data.get();
			items_meta_slk = meta.get();
			items_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
			items_meta_slk.build_meta_map();

			merge_inis(items_slk, items_meta_slk, inis);
		});

		auto doodads_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Doodads/Doodads.slk");
			auto meta = start_loading<SLK>("Doodads/DoodadMetaData.slk");
			auto skin = start_loading<INI>("Doodads/DoodadSkins.txt");

			doodads_slk = data.get();
			doodads_meta_slk = meta.get();
			doodads_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
			doodads_meta_slk.build_meta_map();

			doodads_slk.merge(skin.get(), doodads_meta_slk);
			doodads_slk.substitute(world_edit_strings, "WorldEditStrings");
			doodads_slk.substitute(world_edit_game_strings, "WorldEditStrings");
			default_pitch_and_roll(doodads_slk);
		});

		auto destructibles_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/DestructableData.slk");
			auto meta = start_loading<SLK>("Units/DestructableMetaData.slk");
			auto skin = start_loading<INI>("Units/DestructableSkin.txt");

			destructibles_slk = data.get();
			destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");

			destructibles_meta_slk = meta.get();
			destructibles_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
			destructibles_meta_slk.build_meta_map();

			destructibles_slk.merge(skin.get(), destructibles_meta_slk);
			destructibles_slk.substitute(world_edit_strings, "WorldEditStrings");
			destructibles_slk.substitute(world_edit_game_strings, "WorldEditStrings");
			default_pitch_and_roll(destructibles_slk);
		});

		auto abilities_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/AbilityData.slk");
			auto meta = start_loading<SLK>("Units/AbilityMetaData.slk");

			abilities_slk = data.get();
			abilities_meta_slk = meta.get();
			abilities_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

			// Patch the SLKs
			abilities_slk.add_column("buttonpos2");
			abilities_slk.add_column("unbuttonpos2");
			abilities_slk.add_column("researchbuttonpos2");
			abilities_meta_slk.set_shadow_data("field", "abpy", "buttonpos2");
			abilities_meta_slk.set_shadow_data("field", "auby", "unbuttonpos2");
			abilities_meta_slk.set_shadow_data("field", "arpy", "researchbuttonpos2");
			abilities_meta_slk.build_meta_map();

			merge_inis(abilities_slk, abilities_meta_slk, ability_inis);
		});

		auto upgrade_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/UpgradeData.slk");
			auto meta = start_loading<SLK>("Units/UpgradeMetaData.slk");
			std::array<std::future<INI>, 14> inis = {
				start_loading<INI>("Units/UpgradeSkin.txt"),
				start_loading<INI>("Units/HumanUpgradeFunc.txt"),
				start_loading<INI>("Units/OrcUpgradeFunc.txt"),
				start_loading<INI>("Units/UndeadUpgradeFunc.txt"),
				start_loading<INI>("Units/NightElfUpgradeFunc.txt"),
				start_loading<INI>("Units/NeutralUpgradeFunc.txt"),
				start_loading<INI>("Units/CampaignUpgradeFunc.txt"),
				start_loading<INI>("Units/CampaignUpgradeStrings.txt"),
				start_loading<INI>("Units/HumanUpgradeStrings.txt"),
				start_loading<INI>("Units/NeutralUpgradeStrings.txt"),
				start_loading<INI>("Units/NightElfUpgradeStrings.txt"),
				start_loading<INI>("Units/OrcUpgradeStrings.txt"),
				start_loading<INI>("Units/UndeadUpgradeStrings.txt"),
				start_loading<INI>("Units/UpgradeSkinStrings.txt"),
			};

			upgrade_slk = data.get();
			upgrade_meta_slk = meta.get();
			upgrade_meta_slk.substitute(world_edit_strings, "WorldEditStrings");

			// Patch the SLKs
			upgrade_slk.add_column("buttonpos2");
			upgrade_meta_slk.set_shadow_data("field", "gbpy", "buttonpos2");
			upgrade_meta_slk.build_meta_map();

			upgrade_slk.merge(ability_skin.get(), upgrade_meta_slk);
			merge_inis(upgrade_slk, upgrade_meta_slk, inis);
		});

		auto buff_future = std::async(std::launch::async, [&] {
			auto data = start_loading<SLK>("Units/AbilityBuffData.slk");
			auto meta = start_loading<SLK>("Units/AbilityBuffMetaData.slk");

			buff_slk = data.get();
			buff_meta_slk = meta.get();
			buff_meta_slk.substitute(world_edit_strings, "WorldEditStrings");
			buff_meta_slk.build_meta_map();

			merge_inis(buff_slk, buff_meta_slk, ability_inis);
		});

		try {
			units_future.get();
			abilities_future.get();
			items_future.get();
			doodads_future.get();
			destructibles_future.get();
			upgrade_future.get();
			buff_future.get();
		} catch (const std::exception& e) {
			std::println("Error loading game data: {}", e.what());
			QMessageBox::critical(
				nullptr,
				"Map loading error",
				QString::fromStdString(std::format("Failed to load game data files:\n{}", e.what()))
			);
			return;
		}

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
		} else {
			triggers.load_empty();
		}

		std::println("Trigger loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Map data
		tilesets.load();
		std::println("Tilesets loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		gameplay_constants.load();

		info.load();
		terrain.load(physics, tilesets);

		std::println("Terrain loading: {:>5}ms", timer.elapsed_ms());
		timer.reset();

		// Pathing Map
		if (hierarchy.map_file_exists("war3map.wpm")) {
			pathing_map.load(terrain.width - 1, terrain.height - 1);
		} else {
			pathing_map.resize((terrain.width - 1) * 4, (terrain.height - 1) * 4);
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

		// Fix doodads that have no/empty category
		for (const auto& [id, index] : doodads_slk.row_headers) {
			if (!doodads_slk.data<std::string_view>("category", id).empty()) {
				continue;
			}
			if (auto found = doodads_slk.shadow_data.find(id); found != doodads_slk.shadow_data.end()) {
				found->second.erase("category");
			}
			if (doodads_slk.data<std::string_view>("category", id).empty()) {
				doodads_slk.base_data[id]["category"] = "O";
			}
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
			regions.load(terrain.offset.x, terrain.offset.y);
		}

		// Cameras
		if (hierarchy.map_file_exists("war3map.w3c")) {
			cameras.load(info.game_version_major, info.game_version_minor, terrain.offset.x, terrain.offset.y);
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

		std::println("Full loading: {:>5}ms", full_timer.elapsed_ms());

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
		tilesets.save();
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

		regions.save(terrain.offset.x, terrain.offset.y);
		info.save(terrain.tileset_id);

		trigger_strings.save();
		triggers.save();
		triggers.save_scripts();
		ScriptMode mode = ScriptMode::jass;
		if (info.lua) {
			mode = ScriptMode::lua;
		}

		const auto result = triggers.generate_map_script(terrain, units, doodads, info, sounds, regions, cameras, tilesets, mode);
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

			if (i.skeleton.sequence_index == -1) {
				return;
			}

			mdx::Extent& extent = i.mesh->mdx->sequences.at(i.skeleton.sequence_index).extent;
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
			if (i.skeleton.sequence_index == -1) {
				return;
			}

			mdx::Extent& extent = i.mesh->mdx->sequences.at(i.skeleton.sequence_index).extent;
			if (!camera.inside_frustrum_transform(extent.minimum, extent.maximum, i.skeleton.matrix)) {
				return;
			}

			i.skeleton.update(delta);
		});
	}

	/// Records the whole map. Expects rendering to have begun with the viewport set and the bindless set bound.
	void render(const VkCommandBuffer cmd, FrameAllocator& allocator) {
		// While switching maps it may happen that render is called before loading has finished.
		if (!loaded) {
			return;
		}

		if (render_regions) {
			const auto* region_brush = dynamic_cast<RegionBrush*>(brush);
			regions.update_render_buffer(region_brush ? &region_brush->selections : nullptr);
		}

		// Wireframe covers the terrain, meshes and water, but not the brush overlays
		const VkPolygonMode polygon_mode = render_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		vkCmdSetPolygonModeEXT(cmd, polygon_mode);

		terrain.render_ground(
			cmd,
			allocator,
			render_pathing,
			render_lighting,
			light_direction,
			brush,
			pathing_map,
			render_regions,
			std::as_bytes(std::span(regions.render_data)),
			static_cast<uint32_t>(regions.render_data.size())
		);

		if (render_doodads) {
			doodads_in_view.resize(doodads.doodads.size());
			std::for_each(std::execution::par_unseq, doodads.doodads.begin(), doodads.doodads.end(), [&](const Doodad& i) {
				doodads_in_view[&i - doodads.doodads.data()] = render_manager.in_view(*i.mesh, i.skeleton, i.skeleton.matrix);
			});
			for (size_t k = 0; k < doodads.doodads.size(); k++) {
				const Doodad& i = doodads.doodads[k];
				if (doodads_in_view[k]) {
					render_manager.queue_render_in_view(*i.mesh, i.skeleton, i.skeleton.matrix, i.color, 0);
				}
				if (render_click_helpers && i.use_click_helper) {
					render_manager.queue_click_helper(i.skeleton.matrix);
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

		// Brush previews are queued with the render manager; selection overlays are drawn over the terrain beneath the meshes
		BrushDrawList brush_draw_list;
		if (render_brush && brush) {
			brush->render(brush_draw_list);
		}
		vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);
		brush_overlay.render(cmd, brush_draw_list.selection_rectangles, brush_draw_list.selection_circles, camera.projection_view);
		vkCmdSetPolygonModeEXT(cmd, polygon_mode);

		render_manager.render(cmd, allocator, render_lighting, light_direction);

		if (render_water) {
			terrain.render_water(cmd, allocator, info, tilesets);
		}
		vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);

	}

	/// Resizes the entire map by expanding/shrinking it from all sides
	/// Handles terrain, pathing map, shadow map and preplaced objects
	/// Also, as per vanilla WE behaviour, clears the entire world undo stack
	void resize(int delta_left, int delta_right, int delta_top, int delta_bottom);

	/// Sets the playable area (shadowed map bounds)
	/// Handles the terrain flags, camera bounds and map info
	/// Also updates the pathing map and deletes units/items which are now out of bounds
	void set_playable_area(int unplayable_left, int unplayable_right, int unplayable_top, int unplayable_bottom);

	/// Returns a list containing all the custom resources in the map folder with how many times they are referenced.
	/// Note that this isn't exhaustive as we cannot detect confidently whether all game file overrides are used.
	/// We also scan the map code for the file path (with forward slashed),
	/// but map code can do arbitrary file loading in ways that are hard to detect such as composing a file path by appending two strings
	std::vector<FileUsage> get_file_usage() const;

  private:
	int update_object_positions(int delta_left, int delta_right, int delta_top, int delta_bottom);
	void reset_map_edge_pathing(
		int old_left,
		int old_right,
		int old_top,
		int old_bottom,
		int new_left,
		int new_right,
		int new_top,
		int new_bottom
	);
};

#include "map.moc"
