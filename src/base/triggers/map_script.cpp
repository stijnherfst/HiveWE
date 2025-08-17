module;

#include <QDir>
#include <QProcess>

module Triggers;

import std;
import INI;
namespace fs = std::filesystem;

void generate_global_variables(
	MapScriptWriter& script,
	std::unordered_map<std::string, std::string>& unit_variables,
	std::unordered_map<std::string, std::string>& destructable_variables,
	const std::vector<Trigger>& triggers,
	const std::vector<TriggerVariable>& variables,
	const ini::INI& trigger_data,
	const Regions& regions,
	const GameCameras& cameras,
	const Sounds& sounds
) {
	if (script.mode == ScriptMode::jass) {
		script.write_ln("globals");
	}

	if (script.mode == ScriptMode::jass) {
		for (const auto& variable : variables) {
			const std::string base_type = trigger_data.data("TriggerTypes", variable.type, 4);
			const std::string type = base_type.empty() ? variable.type : base_type;
			if (variable.is_array) {
				script.write_ln(std::format("{} array udg_{}", type, variable.name));
			} else {
				std::string default_value = trigger_data.data("TriggerTypeDefaults", type);

				if (default_value.empty()) { // handle?
					default_value = "null";
				}

				script.global(type, "udg_" + variable.name, default_value);
			}
		}
	} else {
		for (const auto& variable : variables) {
			if (variable.is_array) {
				script.global(variable.type, "udg_" + variable.name, "__jarray(\"\")");
			} else {
				script.global(variable.type, "udg_" + variable.name, script.null());
			}
		}
	}

	for (const auto& i : regions.regions) {
		std::string region_name = i.name;
		trim(region_name);
		std::replace(region_name.begin(), region_name.end(), ' ', '_');
		script.global("rect", "gg_rct_" + region_name, script.null());
	}

	for (const auto& i : cameras.cameras) {
		std::string camera_name = i.name;
		trim(camera_name);
		std::replace(camera_name.begin(), camera_name.end(), ' ', '_');
		script.global("camerasetup", "gg_cam_" + camera_name, script.null());
	}

	for (const auto& i : sounds.sounds) {
		std::string sound_name = i.name;
		trim(sound_name);
		std::replace(sound_name.begin(), sound_name.end(), ' ', '_');

		if (i.music) {
			// Music files are stored as just a file path string
			script.global("string", sound_name, "\"\"");
		} else {
			script.global("sound", sound_name, script.null());
		}
	}

	for (const auto& i : triggers) {
		if (i.is_comment || !i.is_enabled) {
			continue;
		}

		std::string trigger_name = i.name;
		trim(trigger_name);
		std::ranges::replace(trigger_name, ' ', '_');
		script.global("trigger", "gg_trg_" + trigger_name, script.null());
	}

	for (const auto& [creation_number, type] : unit_variables) {
		script.global("unit", "gg_unit_" + type + "_" + creation_number, script.null());
	}

	for (const auto& [creation_number, type] : destructable_variables) {
		script.global("destructable", "gg_dest_" + type + "_" + creation_number, script.null());
	}

	if (script.mode == ScriptMode::jass) {
		script.write_ln("endglobals");
	}
}

void generate_init_global_variables(MapScriptWriter& script, const std::vector<TriggerVariable>& variables, const ini::INI& trigger_data) {
	script.function("InitGlobals", [&]() {
		script.local("integer", "i", "0");

		for (const auto& variable : variables) {
			const std::string base_type = trigger_data.data("TriggerTypes", variable.type, 4);
			const std::string type = base_type.empty() ? variable.type : base_type;
			const std::string default_value = trigger_data.data("TriggerTypeDefaults", type);

			if (!variable.is_initialized && default_value.empty()) {
				continue;
			}

			std::string value;
			if (variable.is_initialized) {
				const std::string converted_value = trigger_data.data("TriggerParams", variable.initial_value, 2);

				if (converted_value.empty()) {
					if (type == "string") {
						value = "\"" + variable.initial_value + "\"";
					} else {
						value = variable.initial_value;
					}
				} else {
					value = converted_value;
				}
			} else {
				if (type == "string") {
					value = "\"\"";
				} else {
					value = default_value;
				}
			}

			if (variable.is_array) {
				script.forloop(0, variable.array_size, "i", [&] {
					script.set_variable("udg_" + variable.name + "[i]", value);
				});
			} else {
				script.set_variable("udg_" + variable.name, value);
			}
		}
	});
}

void generate_units(
	MapScriptWriter& script,
	const std::unordered_map<std::string, std::string>& unit_variables,
	const Terrain& terrain,
	const Units& units
) {
	script.function("CreateAllUnits", [&]() {
		script.local("unit", "u", script.null());
		script.local("integer", "unitID", "0");
		script.local("trigger", "t", script.null());
		script.local("real", "life", "0");

		for (const auto& i : units.units) {
			if (i.id == "sloc") {
				continue;
			}

			std::string unit_reference = "u";
			if (unit_variables.contains(std::format("{:0>4}", i.creation_number))) {
				unit_reference = std::format("gg_unit_{}_{:0>4}", i.id, i.creation_number);
			}

			script.set_variable(
				unit_reference,
				std::format(
					"BlzCreateUnitWithSkin(Player({}), {}, {:.4f}, {:.4f}, {:.4f}, {})",
					i.player,
					script.four_cc(i.id),
					i.position.x * 128.f + terrain.offset.x,
					i.position.y * 128.f + terrain.offset.y,
					glm::degrees(i.angle),
					script.four_cc(i.skin_id)
				)
			);

			if (i.health != -1) {
				script.set_variable("life", std::format("GetUnitState({}, {})", unit_reference, "UNIT_STATE_LIFE"));
				script.call("SetUnitState", unit_reference, "UNIT_STATE_LIFE", std::to_string(i.health / 100.f) + " * life");
			}

			if (i.mana != -1) {
				script.call("SetUnitState", unit_reference, "UNIT_STATE_MANA", i.mana);
			}
			if (i.level != 1) {
				script.call("SetHeroLevel", unit_reference, i.level, "false");
			}

			if (i.strength != 0) {
				script.call("SetHeroStr", unit_reference, i.strength, "true");
			}

			if (i.agility != 0) {
				script.call("SetHeroAgi", unit_reference, i.agility, "true");
			}

			if (i.intelligence != 0) {
				script.call("SetHeroInt", unit_reference, i.intelligence, "true");
			}

			float range;
			if (i.target_acquisition != -1.f) {
				if (i.target_acquisition == -2.f) {
					range = 200.f;
				} else {
					range = i.target_acquisition;
				}
				script.call("SetUnitAcquireRange", unit_reference, range);
			}

			for (const auto& j : i.abilities) {
				for (size_t k = 0; k < std::get<2>(j); k++) {
					script.call("SelectHeroSkill", unit_reference, script.four_cc(std::get<0>(j)));
				}

				if (std::get<1>(j)) {
					std::string order_on = abilities_slk.data("orderon", std::get<0>(j));
					if (order_on.empty()) {
						order_on = abilities_slk.data("order", std::get<0>(j));
					}
					script.call("IssueImmediateOrder", unit_reference, "\"" + order_on + "\"");

				} else {
					std::string order_off = abilities_slk.data("orderoff", std::get<0>(j));
					if (!order_off.empty()) {
						script.call("IssueImmediateOrder", unit_reference, "\"" + order_off + "\"");
					}
				}
			}

			for (const auto& j : i.items) {
				script.call("UnitAddItemToSlotById", unit_reference, script.four_cc(j.second), j.first);
			}

			if (i.item_sets.size()) {
				script.set_variable("t", "CreateTrigger()");
				script.call("TriggerRegisterUnitEvent", "t", unit_reference, "EVENT_UNIT_DEATH");
				script.call("TriggerRegisterUnitEvent", "t", unit_reference, "EVENT_UNIT_CHANGE_OWNER");

				if (script.mode == ScriptMode::jass) {
					script.call("TriggerAddAction", "t", "function UnitItemDrops_" + std::to_string(i.creation_number));
				} else {
					script.call("TriggerAddAction", "t", "UnitItemDrops_" + std::to_string(i.creation_number));
				}
			}
		}
	});
}

void generate_items(MapScriptWriter& script, const Terrain& terrain, const Units& units) {
	script.function("CreateAllItems", [&]() {
		for (const auto& i : units.items) {
			script.call(
				"BlzCreateItemWithSkin",
				script.four_cc(i.id),
				i.position.x * 128.f + terrain.offset.x,
				i.position.y * 128.f + terrain.offset.y,
				script.four_cc(i.id)
			);
		}
	});
}

void generate_destructables(
	MapScriptWriter& script,
	const std::unordered_map<std::string, std::string>& destructable_variables,
	const Terrain& terrain,
	const Doodads& doodads
) {
	script.function("CreateAllDestructables", [&]() {
		script.local("destructable", "d", script.null());
		script.local("trigger", "t", script.null());
		script.local("real", "life", "0");

		for (const auto& i : doodads.doodads) {
			std::string id = "d";

			if (destructable_variables.contains(std::to_string(i.creation_number))) {
				id = "gg_dest_" + i.id + "_" + std::to_string(i.creation_number);
			}

			if (id == "d" && i.item_sets.empty() && i.item_table_pointer == -1) {
				continue;
			}

			script.set_variable(
				id,
				std::format(
					"BlzCreateDestructableZWithSkin({}, {:.4f}, {:.4f}, {:.4f}, {}, {}, {}, {})",
					script.four_cc(i.id),
					i.position.x * 128.f + terrain.offset.x,
					i.position.y * 128.f + terrain.offset.y,
					i.position.z * 128.f,
					glm::degrees(i.angle),
					i.scale.x,
					i.variation,
					script.four_cc(i.skin_id)
				)
			);

			if (i.life != 100) {
				script.set_variable("life", "GetDestructableLife(" + id + ")");
				script.call("SetDestructableLife", id, std::to_string(i.life / 100.f) + " * life");
			}

			if (!i.item_sets.empty()) {
				script.set_variable("t", "CreateTrigger()");
				script.call("TriggerRegisterDeathEvent", "t", id);
				if (script.mode == ScriptMode::jass) {
					script.call("TriggerAddAction", "t", "function SaveDyingWidget");
				} else {
					script.call("TriggerAddAction", "t", "SaveDyingWidget");
				}
				if (script.mode == ScriptMode::jass) {
					script.call("TriggerAddAction", "t", "function DoodadItemDrops_" + std::to_string(i.creation_number));
				} else {
					script.call("TriggerAddAction", "t", "DoodadItemDrops_" + std::to_string(i.creation_number));
				}
			} else if (i.item_table_pointer != -1) {
				script.set_variable("t", "CreateTrigger()");
				script.call("TriggerRegisterDeathEvent", "t", id);
				if (script.mode == ScriptMode::jass) {
					script.call("TriggerAddAction", "t", "function SaveDyingWidget");
					script.call("TriggerAddAction", "t", "function ItemTable_" + std::to_string(i.item_table_pointer));
				} else {
					script.call("TriggerAddAction", "t", "SaveDyingWidget");
					script.call("TriggerAddAction", "t", "ItemTable_" + std::to_string(i.item_table_pointer));
				}
			}
		}
	});
}

void generate_regions(MapScriptWriter& script, const Regions& regions) {
	script.function("CreateRegions", [&]() {
		script.local("weathereffect", "we", script.null());
		for (const auto& i : regions.regions) {
			std::string region_name = "gg_rct_" + i.name;
			trim(region_name);
			std::ranges::replace(region_name, ' ', '_');

			script.set_variable(
				region_name,
				std::format(
					"Rect({}, {}, {}, {})",
					std::min(i.left, i.right),
					std::min(i.bottom, i.top),
					std::max(i.left, i.right),
					std::max(i.bottom, i.top)
				)
			);

			if (!i.weather_id.empty()) {
				script.set_variable("we", std::format("AddWeatherEffect({}, {})", region_name, script.four_cc(i.weather_id)));
				script.call("EnableWeatherEffect", "we", true);
			}
		}
	});
}

void generate_cameras(MapScriptWriter& script, const GameCameras& cameras) {
	script.function("CreateCameras", [&]() {
		for (const auto& i : cameras.cameras) {
			std::string camera_name = "gg_cam_" + i.name;
			trim(camera_name);
			std::ranges::replace(camera_name, ' ', '_');

			script.set_variable(camera_name, "CreateCameraSetup()");
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ZOFFSET", i.z_offset, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ROTATION", i.rotation, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ANGLE_OF_ATTACK", i.angle_of_attack, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_TARGET_DISTANCE", i.distance, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ROLL", i.roll, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_FIELD_OF_VIEW", i.fov, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_FARZ", i.far_z, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_NEARZ", i.near_z, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_PITCH", i.local_pitch, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_YAW", i.local_yaw, 0.0);
			script.call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_ROLL", i.local_roll, 0.0);
			script.call("CameraSetupSetDestPosition", camera_name, i.target_x, i.target_y, 0.0);
		}
	});
}

// Todo, missing fields, soundduration also wrong
void generate_sounds(MapScriptWriter& script, const Sounds& sounds) {
	script.function("InitSounds", [&]() {
		for (const auto& i : sounds.sounds) {
			std::string sound_name = i.name;
			trim(sound_name);
			std::ranges::replace(sound_name, ' ', '_');

			if (i.music) {
				// I suppose music files can't be muted?
				script.set_variable(sound_name, "\"" + string_replaced(i.file, "\\", "\\\\") + "\"");
			} else {
				script.set_variable(
					sound_name,
					std::format(
						"CreateSound(\"{}\", {}, {}, {}, {}, {}, \"{}\")",
						string_replaced(i.file, "\\", "\\\\"),
						i.looping ? "true" : "false",
						i.is_3d ? "true" : "false",
						i.stop_out_of_range ? "true" : "false",
						i.fade_in_rate,
						i.fade_out_rate,
						string_replaced(i.eax_effect, "\\", "\\\\")
					)
				);

				script.call("SetSoundDuration", sound_name, i.fade_in_rate);
				script.call("SetSoundChannel", sound_name, i.channel);
				script.call("SetSoundVolume", sound_name, i.volume);
				script.call("SetSoundPitch", sound_name, i.pitch);
			}
		}
	});
}

void generate_trigger_initialization(
	MapScriptWriter& script,
	const std::vector<std::string>& initialization_triggers,
	const std::vector<Trigger>& triggers
) {
	script.function("InitCustomTriggers", [&]() {
		for (const auto& i : triggers) {
			if (i.is_comment || !i.is_enabled) {
				continue;
			}
			std::string trigger_name = i.name;
			trim(trigger_name);
			std::replace(trigger_name.begin(), trigger_name.end(), ' ', '_');

			script.call("InitTrig_" + trigger_name);
		}
	});

	script.function("RunInitializationTriggers", [&]() {
		for (const auto& i : initialization_triggers) {
			script.call("ConditionalTriggerExecute", i);
		}
	});
}

void generate_players(MapScriptWriter& script, const MapInfo& map_info) {
	script.function("InitCustomPlayerSlots", [&]() {
		const std::vector<std::string> players =
			{"MAP_CONTROL_USER", "MAP_CONTROL_COMPUTER", "MAP_CONTROL_NEUTRAL", "MAP_CONTROL_RESCUABLE"};
		const std::vector<std::string> races =
			{"RACE_PREF_RANDOM", "RACE_PREF_HUMAN", "RACE_PREF_ORC", "RACE_PREF_UNDEAD", "RACE_PREF_NIGHTELF"};

		size_t index = 0;
		for (const auto& i : map_info.players) {
			std::string player = "Player(" + std::to_string(i.internal_number) + ")";

			script.call("SetPlayerStartLocation", player, index);
			if (i.fixed_start_position || i.race == PlayerRace::selectable) {
				script.call("ForcePlayerStartLocation", player, index);
			}

			script.call("SetPlayerColor", player, "ConvertPlayerColor(" + std::to_string(i.internal_number) + ")");
			script.call("SetPlayerRacePreference", player, races[static_cast<int>(i.race)]);
			script.call("SetPlayerRaceSelectable", player, true);
			script.call("SetPlayerController", player, players[static_cast<int>(i.type)]);

			if (i.type == PlayerType::rescuable) {
				for (const auto& j : map_info.players) {
					if (j.type == PlayerType::human) {
						script.call(
							"SetPlayerAlliance",
							player,
							"Player(" + std::to_string(j.internal_number) + ")",
							"ALLIANCE_RESCUABLE",
							true
						);
					}
				}
			}

			script.write("\n");
			index++;
		}
	});
}

static void write_item_table_entry(MapScriptWriter& script, int chance, const std::string& id) {
	if (id.empty()) {
		script.call("RandomDistAddItem", -1, chance);
	} else if (id[0] == 'Y' && id[2] == 'I' && ((id[1] >= 'i' && id[1] <= 'o') || id[1] == 'Y')) { // Random items

		std::string item_type;
		switch (id[1]) {
			case 'i':
				item_type = "PERMANENT";
				break;
			case 'j':
				item_type = "CHARGED";
				break;
			case 'k':
				item_type = "POWERUP";
				break;
			case 'l':
				item_type = "ARTIFACT";
				break;
			case 'm':
				item_type = "PURCHASABLE";
				break;
			case 'n':
				item_type = "CAMPAIGN";
				break;
			case 'o':
				item_type = "MISCELLANEOUS";
				break;
			case 'Y':
				item_type = "ANY";
				break;
			default:
				std::println("Error: unknown random item type {}", id[1]);
		}

		const std::string random_item =
			std::format("ChooseRandomItemEx(ITEM_TYPE_{}, {})", item_type, (id[3] == '/') ? "-1" : std::string(1, id[3]));
		script.call("RandomDistAddItem", random_item, chance);
	} else {
		script.call("RandomDistAddItem", script.four_cc(id), chance);
	}
}

template<typename T>
void generate_item_tables(MapScriptWriter& script, const std::string& table_name_prefix, const std::vector<T>& table_holders) {
	for (const auto& i : table_holders) {
		if (i.item_sets.empty()) {
			continue;
		}

		script.function(table_name_prefix + std::to_string(i.creation_number), [&]() {
			script.local("widget", "trigWidget", script.null());
			script.local("unit", "trigUnit", script.null());
			script.local("integer", "itemID", "0");
			script.local("boolean", "canDrop", "true");

			script.set_variable("trigWidget", "bj_lastDyingWidget");

			script.if_statement("trigWidget == " + script.null(), [&] {
				script.set_variable("trigUnit", "GetTriggerUnit()");
			});

			script.if_statement("not(trigUnit == " + script.null() + ")", [&]() {
				script.set_variable("canDrop", "not IsUnitHidden(trigUnit)");
				script.if_statement("canDrop and not(GetChangingUnit() == " + script.null() + ")", [&]() {
					script.set_variable("canDrop", "(GetChangingUnitPrevOwner() == Player(PLAYER_NEUTRAL_AGGRESSIVE))");
				});
			});

			script.if_statement("canDrop", [&]() {
				for (const auto& j : i.item_sets) {
					script.call("RandomDistReset");
					for (const auto& [chance, id] : j.items) {
						write_item_table_entry(script, chance, id);
					}
					script.set_variable("itemID", "RandomDistChoose()");
					script.if_statement("not(trigUnit == " + script.null() + ")", [&]() {
						script.call("UnitDropItem", "trigUnit", "itemID"); // Todo fourcc?
					});
					script.if_statement("trigUnit == " + script.null(), [&]() {
						script.call("WidgetDropItem", "trigWidget", "itemID"); // Todo fourcc?
					});
				}
			});

			script.set_variable("bj_lastDyingWidget", script.null());
			script.call("DestroyTrigger", "GetTriggeringTrigger()");
		});
	}
}

void generate_custom_teams(MapScriptWriter& script, const MapInfo& map_info) {
	script.function("InitCustomTeams", [&]() {
		int current_force = 0;
		for (const auto& i : map_info.forces) {
			for (const auto& j : map_info.players) {
				if (i.player_masks & (1 << j.internal_number)) {
					script.call("SetPlayerTeam", "Player(" + std::to_string(j.internal_number) + ")", current_force);

					if (i.allied_victory) {
						script
							.call("SetPlayerState", "Player(" + std::to_string(j.internal_number) + ")", "PLAYER_STATE_ALLIED_VICTORY", 1);
					}
				}
			}

			for (const auto& j : map_info.players) {
				if (i.player_masks & (1 << j.internal_number)) {
					for (const auto& k : map_info.players) {
						if (i.player_masks & (1 << k.internal_number) && j.internal_number != k.internal_number) {
							if (i.allied) {
								script.call(
									"SetPlayerAllianceStateAllyBJ",
									"Player(" + std::to_string(j.internal_number) + ")",
									"Player(" + std::to_string(k.internal_number) + ")",
									true
								);
							}
							if (i.share_vision) {
								script.call(
									"SetPlayerAllianceStateVisionBJ",
									"Player(" + std::to_string(j.internal_number) + ")",
									"Player(" + std::to_string(k.internal_number) + ")",
									true
								);
							}
							if (i.share_unit_control) {
								script.call(
									"SetPlayerAllianceStateControlBJ",
									"Player(" + std::to_string(j.internal_number) + ")",
									"Player(" + std::to_string(k.internal_number) + ")",
									true
								);
							}
							if (i.share_advanced_unit_control) {
								script.call(
									"SetPlayerAllianceStateFullControlBJ",
									"Player(" + std::to_string(j.internal_number) + ")",
									"Player(" + std::to_string(k.internal_number) + ")",
									true
								);
							}
						}
					}
				}
			}
			current_force++;
		}
	});
}

void generate_ally_priorities(MapScriptWriter& script, const MapInfo& map_info) {
	script.function("InitAllyPriorities", [&]() {
		std::unordered_map<int, int> player_to_startloc;

		int current_player = 0;
		for (const auto& i : map_info.players) {
			player_to_startloc[i.internal_number] = current_player;
			current_player++;
		}

		current_player = 0;
		for (const auto& i : map_info.players) {
			size_t count = 0;
			for (const auto& j : map_info.players) {
				if (i.ally_low_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					count++;
				} else if (i.ally_high_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					count++;
				}
			}

			script.call("SetStartLocPrioCount", current_player, count);

			size_t current_index = 0;
			for (const auto& j : map_info.players) {
				if (i.ally_low_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					script
						.call("SetStartLocPrio", current_player, current_index, player_to_startloc[j.internal_number], "MAP_LOC_PRIO_LOW");
					current_index++;
				} else if (i.ally_high_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					script
						.call("SetStartLocPrio", current_player, current_index, player_to_startloc[j.internal_number], "MAP_LOC_PRIO_HIGH");
					current_index++;
				}
			}

			current_player++;
		}
	});
}

void generate_main(MapScriptWriter& script, const Terrain& terrain, const MapInfo& map_info) {
	script.function("main", [&]() {
		script.call(
			"SetCameraBounds",
			std::to_string(map_info.camera_left_bottom.x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT)",
			std::to_string(map_info.camera_left_bottom.y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM)",

			std::to_string(map_info.camera_right_top.x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT)",
			std::to_string(map_info.camera_right_top.y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP)",

			std::to_string(map_info.camera_left_top.x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT)",
			std::to_string(map_info.camera_left_top.y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP)",

			std::to_string(map_info.camera_right_bottom.x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT)",
			std::to_string(map_info.camera_right_bottom.y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM)"
		);

		const std::string terrain_lights = string_replaced(world_edit_data.data("TerrainLights", ""s + terrain.tileset), "\\", "/");
		const std::string unit_lights = string_replaced(world_edit_data.data("TerrainLights", ""s + terrain.tileset), "\\", "/");
		script.call("SetDayNightModels", "\"" + terrain_lights + "\"", "\"" + unit_lights + "\"");

		const std::string sound_environment = string_replaced(world_edit_data.data("SoundEnvironment", ""s + terrain.tileset), "\\", "/");
		script.call("NewSoundEnvironment", "\"" + sound_environment + "\"");

		const std::string ambient_day = string_replaced(world_edit_data.data("DayAmbience", ""s + terrain.tileset), "\\", "/");
		script.call("SetAmbientDaySound", "\"" + ambient_day + "\"");

		const std::string ambient_night = string_replaced(world_edit_data.data("NightAmbience", ""s + terrain.tileset), "\\", "/");
		script.call("SetAmbientNightSound", "\"" + ambient_night + "\"");

		script.call("SetMapMusic", "\"Music\"", true, 0);
		script.call("InitSounds");
		script.call("CreateRegions");
		script.call("CreateCameras");
		script.call("CreateAllDestructables");
		script.call("CreateAllItems");
		script.call("CreateAllUnits");
		script.call("InitBlizzard");
		script.call("InitGlobals");
		script.call("InitCustomTriggers");
		script.call("RunInitializationTriggers");
	});
}

void generate_map_configuration(MapScriptWriter& script, const Terrain& terrain, const Units& units, const MapInfo& map_info) {
	script.function("config", [&]() {
		script.call("SetMapName", "\"" + map_info.name + "\"");
		script.call("SetMapDescription", "\"" + map_info.description + "\"");
		script.call("SetPlayers", map_info.players.size());
		script.call("SetTeams", map_info.forces.size());
		script.call("SetGamePlacement", "MAP_PLACEMENT_USE_MAP_SETTINGS");

		script.write("\n");

		for (const auto& i : units.units) {
			if (i.id == "sloc") {
				script.call(
					"DefineStartLocation",
					i.player,
					i.position.x * 128.f + terrain.offset.x,
					i.position.y * 128.f + terrain.offset.y
				);
			}
		}

		script.write("\n");

		script.call("InitCustomPlayerSlots");
		if (map_info.custom_forces) {
			script.call("InitCustomTeams");
		} else {
			for (const auto& i : map_info.players) {
				script.call("SetPlayerSlotAvailable", "Player(" + std::to_string(i.internal_number) + ")", "MAP_CONTROL_USER");
			}

			script.call("InitGenericPlayerSlots");
		}
		script.call("InitAllyPriorities");
	});
}

/// Returns compile output which could contain errors or general information
std::expected<void, std::string> Triggers::generate_map_script(
	const Terrain& terrain,
	const Units& units,
	const Doodads& doodads,
	const MapInfo& map_info,
	const Sounds& sounds,
	const Regions& regions,
	const GameCameras& cameras,
	ScriptMode mode
) {
	std::unordered_map<std::string, std::string> unit_variables; // creation_number, unit_id
	std::unordered_map<std::string, std::string> destructable_variables; // creation_number, destructable_id
	std::vector<std::string> initialization_triggers;

	std::string trigger_script;
	for (const auto& i : triggers) {
		if (i.is_comment || !i.is_enabled) {
			continue;
		}
		if (!i.custom_text.empty()) {
			trigger_script += i.custom_text + "\n";
		} else {
			trigger_script += convert_gui_to_jass(i, initialization_triggers, mode);
		}
	}

	// Search the trigger script for global unit/destructible definitions
	size_t pos = trigger_script.find("gg_unit", 0);
	while (pos != std::string::npos) {
		std::string type = trigger_script.substr(pos + 8, 4);
		std::string creation_number = trigger_script.substr(pos + 13, 4);
		unit_variables[creation_number] = type;
		pos = trigger_script.find("gg_unit", pos + 17);
	}

	pos = trigger_script.find("gg_dest", 0);
	while (pos != std::string::npos) {
		std::string type = trigger_script.substr(pos + 8, 4);
		std::string creation_number = trigger_script.substr(pos + 13, trigger_script.find_first_not_of("0123456789", pos + 13) - pos - 13);
		destructable_variables[creation_number] = type;
		pos = trigger_script.find("gg_dest", pos + 17);
	}

	MapScriptWriter script_writer(mode);

	generate_global_variables(
		script_writer,
		unit_variables,
		destructable_variables,
		triggers,
		variables,
		trigger_data,
		regions,
		cameras,
		sounds
	);
	generate_init_global_variables(script_writer, variables, trigger_data);
	generate_item_tables(script_writer, "ItemTable_", map_info.random_item_tables);
	generate_item_tables(script_writer, "UnitItemDrops_", units.units);
	generate_item_tables(script_writer, "DoodadItemDrops_", doodads.doodads);
	generate_sounds(script_writer, sounds);

	generate_destructables(script_writer, destructable_variables, terrain, doodads);
	generate_items(script_writer, terrain, units);
	generate_units(script_writer, unit_variables, terrain, units);
	generate_regions(script_writer, regions);
	generate_cameras(script_writer, cameras);

	script_writer.write_ln(global_jass);

	script_writer.write(trigger_script);

	generate_trigger_initialization(script_writer, initialization_triggers, triggers);
	generate_players(script_writer, map_info);
	generate_custom_teams(script_writer, map_info);
	generate_ally_priorities(script_writer, map_info);
	generate_main(script_writer, terrain, map_info);
	generate_map_configuration(script_writer, terrain, units, map_info);

	fs::path path = QDir::tempPath().toStdString() + "/input.lua";
	std::ofstream output(path, std::ios::binary);
	output.write((char*)script_writer.script.data(), script_writer.script.size());
	output.close();

	if (mode == ScriptMode::jass) {
		QProcess* proc = new QProcess();
		proc->setWorkingDirectory("data/tools");
		proc->start(
			"data/tools/clijasshelper.exe",
			{"--scriptonly", "common.j", "blizzard.j", QString::fromStdString(path.string()), "war3map.j"}
		);
		proc->waitForFinished();
		const QString result = proc->readAllStandardOutput();

		if (result.contains("Compile error")) {
			return std::unexpected(result.mid(result.indexOf("Compile error")).toStdString());
		} else if (result.contains("compile errors")) {
			return std::unexpected(result.mid(result.indexOf("compile errors.")).toStdString());
		} else {
			hierarchy.map_file_add("data/tools/war3map.j", "war3map.j");
		}
	} else {
		hierarchy.map_file_add(path, "war3map.lua");
	}

	return {};
}