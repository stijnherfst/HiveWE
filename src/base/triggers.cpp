#include "triggers.h"

#include <functional>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <QProcess>
#include <QMessageBox>
#include <QDir>

#include <format>
#include <print>

#include "globals.h"
#include <map_global.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

import Hierarchy;
import Utilities;

using namespace std::literals::string_literals;

void Triggers::parse_parameter_structure(BinaryReader& reader, TriggerParameter& parameter, uint32_t version) {
	parameter.type = static_cast<TriggerParameter::Type>(reader.read<uint32_t>());
	parameter.value = reader.read_c_string();
	parameter.has_sub_parameter = reader.read<uint32_t>();
	if (parameter.has_sub_parameter) {
		parameter.sub_parameter.type = static_cast<TriggerSubParameter::Type>(reader.read<uint32_t>());
		parameter.sub_parameter.name = reader.read_c_string();
		parameter.sub_parameter.begin_parameters = reader.read<uint32_t>();
		if (parameter.sub_parameter.begin_parameters) {
			parameter.sub_parameter.parameters.resize(argument_counts[parameter.sub_parameter.name]);
			for (auto&& i : parameter.sub_parameter.parameters) {
				parse_parameter_structure(reader, i, version);
			}
		}
	}
	if (version == 4) {
		if (parameter.type == TriggerParameter::Type::function) {
			reader.advance(4); // Unknown always 0
		} else {
			parameter.is_array = reader.read<uint32_t>();
		}
	} else {
		if (parameter.has_sub_parameter) {
			parameter.unknown = reader.read<uint32_t>(); // Unknown always 0
		}
		parameter.is_array = reader.read<uint32_t>();
	}
	if (parameter.is_array) {
		parameter.parameters.resize(1);
		parse_parameter_structure(reader, parameter.parameters.front(), version);
	}
}

void Triggers::parse_eca_structure(BinaryReader& reader, ECA& eca, bool is_child, uint32_t version) {
	eca.type = static_cast<ECA::Type>(reader.read<uint32_t>());
	if (is_child) {
		eca.group = reader.read<uint32_t>();
	}
	eca.name = reader.read_c_string();
	eca.enabled = reader.read<uint32_t>();
	eca.parameters.resize(argument_counts[eca.name]);
	for (auto&& i : eca.parameters) {
		parse_parameter_structure(reader, i, version);
	}
	if (version == 7) {
		eca.ecas.resize(reader.read<uint32_t>());
		for (auto&& i : eca.ecas) {
			parse_eca_structure(reader, i, true, version);
		}
	}
}

void Triggers::load() {
	BinaryReader reader = hierarchy.map_file_read("war3map.wtg");

	trigger_strings.load("UI/TriggerStrings.txt");
	trigger_data.load("UI/TriggerData.txt");
	trigger_data.substitute(world_edit_strings, "WorldEditStrings");

	// Manual fixes
	trigger_data.set_whole_data("TriggerTypeDefaults", "string", "\"\"");

	for (auto&& section : { "TriggerActions"s, "TriggerEvents"s, "TriggerConditions"s, "TriggerCalls"s }) {
		for (const auto& [key, value] : trigger_data.section(section)) {
			if (key.front() == '_') {
				continue;
			}

			int arguments = 0;
			for (const auto& j : value) {
				arguments += !j.empty() && !is_number(j) && j != "nothing";
			}

			if (section == "TriggerCalls") {
				--arguments;
			}

			argument_counts[key] = arguments;
		}
	}

	Trigger::next_id = 0;

	std::string magic_number = reader.read_string(4);
	if (magic_number != "WTG!") {
		std::print("Unknown magic number for war3map.wtg {}\n", magic_number);
		return;
	}

	uint32_t version = reader.read<uint32_t>();
	if (version == 0x80000004)
		load_version_31(reader, version);
	else if (version == 4 || version == 7)
		load_version_pre31(reader, version);
	else {
		std::print("Unknown WTG format! Trying 1.31 loader\n");
		load_version_31(reader, version);
	}
}

void Triggers::load_version_pre31(BinaryReader& reader, uint32_t version) {
	std::print("Importing pre-1.31 trigger format\n");

	categories.resize(reader.read<uint32_t>());
	for (auto& i : categories) {
		i.classifier = Classifier::category;
		i.id = reader.read<uint32_t>();
		i.name = reader.read_c_string();
		i.parent_id = 0;
		if (version == 7) {
			i.is_comment = reader.read<uint32_t>();
		}

		Trigger::next_id = std::max(Trigger::next_id, i.id + 1);
		if (i.id == 0) {
			i.id = -2;
		}
	}


	reader.advance(4); // dunno

	int variable_category = Trigger::next_id++;
	categories.insert(categories.begin(), { Classifier::map, 0, "Map Header", true, false, -1 });
	categories.insert(categories.begin(), { Classifier::category, variable_category, "Variables", true, false, 0 });

	variables.resize(reader.read<uint32_t>());
	for (auto& i : variables) {
		i.name = reader.read_c_string();
		i.type = reader.read_c_string();
		i.unknown = reader.read<uint32_t>();
		i.id = Trigger::next_id++;

		i.is_array = reader.read<uint32_t>();
		if (version == 7) {
			i.array_size = reader.read<uint32_t>();
		}
		i.is_initialized = reader.read<uint32_t>();
		i.initial_value = reader.read_c_string();
		i.parent_id = variable_category;
	}

	triggers.resize(reader.read<uint32_t>());
	for (auto& i : triggers) {
		i.name = reader.read_c_string();
		i.description = reader.read_c_string();
		if (version == 7) {
			i.is_comment = reader.read<uint32_t>();
		}
		i.is_enabled = reader.read<uint32_t>();
		i.is_script = reader.read<uint32_t>();
		i.initially_on = !reader.read<uint32_t>();
		i.run_on_initialization = reader.read<uint32_t>();

		i.id = Trigger::next_id++;

		if (i.run_on_initialization && i.is_script) {
			i.classifier = Classifier::gui;
		} else if (i.is_comment) {
			i.classifier = Classifier::comment;
		} else if (i.is_script) {
			i.classifier = Classifier::script;
		} else {
			i.classifier = Classifier::gui;
		}
    
		i.parent_id = reader.read<uint32_t>();
		if (i.parent_id == 0) {
			i.parent_id = -2;
		}
		i.ecas.resize(reader.read<uint32_t>());
		for (auto& j : i.ecas) {
			parse_eca_structure(reader, j, false, version);
		}
	}
}

void Triggers::load_version_31(BinaryReader& reader, uint32_t version) {
	uint32_t sub_version = reader.read<uint32_t>();
	if (sub_version != 7 && sub_version != 4) {
		std::print("Unknown 1.31 WTG subformat! Trying anyway.\n");
	}

	reader.advance(4);							 // map_count
	reader.advance(4 * reader.read<uint32_t>()); //map ids of deleted maps

	reader.advance(4);							 // library_count
	reader.advance(4 * reader.read<uint32_t>()); // library ids of deleted libraries
	
	reader.advance(4);							 // category_count
	reader.advance(4 * reader.read<uint32_t>()); // category ids of deleted categories

	reader.advance(4);							 // trigger_count
	reader.advance(4 * reader.read<uint32_t>()); // trigger ids of deleted triggers

	reader.advance(4);							 // comment_count
	reader.advance(4 * reader.read<uint32_t>()); // comment ids of deleted comments

	reader.advance(4);							 // script_count
	reader.advance(4 * reader.read<uint32_t>()); // script ids of deleted scripts

	reader.advance(4);							 // variable_count
	reader.advance(4 * reader.read<uint32_t>()); // variable ids of deleted variables

	unknown1 = reader.read<uint32_t>();
	unknown2 = reader.read<uint32_t>();
	trig_def_ver = reader.read<uint32_t>();

	uint32_t variable_count = reader.read<uint32_t>();
	for (uint32_t i = 0; i < variable_count; i++) {
		TriggerVariable variable;
		variable.name = reader.read_c_string();
		variable.type = reader.read_c_string();
		variable.unknown = reader.read<uint32_t>();
		variable.is_array = reader.read<uint32_t>();
		if (sub_version == 7) {
			variable.array_size = reader.read<uint32_t>();
		}
		variable.is_initialized = reader.read<uint32_t>();
		variable.initial_value = reader.read_c_string();
		variable.id = reader.read<uint32_t>();
		variable.parent_id = reader.read<uint32_t>();
		variables.push_back(variable);

		Trigger::next_id = std::max(Trigger::next_id, variable.id + 1);
	}
	
	uint32_t element_count = reader.read<uint32_t>();

	for (uint32_t i = 0; i < element_count; i++) {
		Classifier classifier = static_cast<Classifier>(reader.read<uint32_t>());
		switch (classifier) {
			case Classifier::map:
			case Classifier::library:
			case Classifier::category: {
				TriggerCategory cat;
				cat.classifier = classifier;
				cat.id = reader.read<uint32_t>();
				cat.name = reader.read_c_string();
				if (sub_version == 7) {
					cat.is_comment = reader.read<uint32_t>();
				}
				cat.open_state = reader.read<uint32_t>();
				cat.parent_id = reader.read<uint32_t>();
				categories.push_back(cat);

				Trigger::next_id = std::max(Trigger::next_id, cat.id + 1);
				break;
			}
			case Classifier::gui:
			case Classifier::comment:
			case Classifier::script: {
				Trigger trigger;
				trigger.classifier = classifier;
				trigger.name = reader.read_c_string();
				trigger.description = reader.read_c_string();
				if (sub_version == 7) {
					trigger.is_comment = reader.read<uint32_t>();
				}
				trigger.id = reader.read<uint32_t>();
				trigger.is_enabled = reader.read<uint32_t>();
				trigger.is_script = reader.read<uint32_t>();
				trigger.initially_on = !reader.read<uint32_t>();
				trigger.run_on_initialization = reader.read<uint32_t>();
				trigger.parent_id = reader.read<uint32_t>();
				trigger.ecas.resize(reader.read<uint32_t>());
				for (auto& j : trigger.ecas) {
					parse_eca_structure(reader, j, false, sub_version);
				}

				triggers.push_back(trigger);

				Trigger::next_id = std::max(Trigger::next_id, trigger.id + 1);
				break;
			}
			case Classifier::variable: {
				reader.advance(4); //id
				reader.advance_c_string(); //name
				reader.advance(4); //parentid
				break;
			}
		}
	}
}

void Triggers::load_jass() {
	BinaryReader reader = hierarchy.map_file_read("war3map.wct");

	const uint32_t version = reader.read<uint32_t>();
	if (version != 0x80000004) {
		if (version == 1 || version == 0) {
			if (version == 1) {
				global_jass_comment = reader.read_c_string();
				global_jass = reader.read_string(reader.read<uint32_t>());
			}
			reader.advance(4);
			for (auto&& i : triggers) {
				const uint32_t size = reader.read<uint32_t>();
				if (size > 0) {
					i.custom_text = reader.read_string(size);
				}
			}
			return;
		} else {
			std::print("Probably invalid WCT format\n");
		}
	} 

	const int sub_version = reader.read<uint32_t>();
	if (sub_version != 1 && sub_version != 0) {
		std::print("Unknown WCT 1.31 subformat\n");
	}

	if (sub_version == 1) {
		global_jass_comment = reader.read_c_string();
		int size = reader.read<uint32_t>();
		if (size > 0) {
			global_jass = reader.read_string(size);
		}
	}

	for (auto& i : triggers) {
		if (!i.is_comment) {
			int size = reader.read<uint32_t>();
			if (size > 0) {
				i.custom_text = reader.read_string(size);
			}
		}
	}
}

void Triggers::print_parameter_structure(BinaryWriter& writer, const TriggerParameter& parameter) const {
	writer.write<uint32_t>(static_cast<int>(parameter.type));
	writer.write_c_string(parameter.value);
	writer.write<uint32_t>(parameter.has_sub_parameter);

	if (parameter.has_sub_parameter) {
		writer.write<uint32_t>(static_cast<int>(parameter.sub_parameter.type));
		writer.write_c_string(parameter.sub_parameter.name);
		writer.write<uint32_t>(parameter.sub_parameter.begin_parameters);
		if (parameter.sub_parameter.begin_parameters) {
			for (const auto& i : parameter.sub_parameter.parameters) {
				print_parameter_structure(writer, i);
			}
		}

		writer.write<uint32_t>(parameter.unknown);
	}
	writer.write<uint32_t>(parameter.is_array);
	if (parameter.is_array) {
		print_parameter_structure(writer, parameter.parameters.front());
	}
}

void Triggers::print_eca_structure(BinaryWriter& writer, const ECA& eca, bool is_child) const {
	writer.write<uint32_t>(static_cast<int>(eca.type));
	if (is_child) {
		writer.write<uint32_t>(eca.group);
	}

	writer.write_c_string(eca.name);
	writer.write<uint32_t>(eca.enabled);
	for (const auto& i : eca.parameters) {
		print_parameter_structure(writer, i);
	}

	writer.write<uint32_t>(eca.ecas.size());
	for (const auto& i : eca.ecas) {
		print_eca_structure(writer, i, true);
	}
}

void Triggers::save() const {
 	BinaryWriter writer;
	writer.write_string("WTG!");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(write_sub_version);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(0);
	writer.write<uint32_t>(0);

	writer.write<uint32_t>(unknown1);
	writer.write<uint32_t>(unknown2);
	writer.write<uint32_t>(trig_def_ver);
	writer.write<uint32_t>(variables.size());

	for (const auto& i : variables) {
		writer.write_c_string(i.name);
		writer.write_c_string(i.type);
		writer.write<uint32_t>(i.unknown);
		writer.write<uint32_t>(i.is_array);
		writer.write<uint32_t>(i.array_size);
		writer.write<uint32_t>(i.is_initialized);
		writer.write_c_string(i.initial_value);
		writer.write<uint32_t>(i.id);
		writer.write<uint32_t>(i.parent_id);
	}

	writer.write<uint32_t>(categories.size() + triggers.size() + variables.size());
	
	for (const auto& i : categories) {
		writer.write<uint32_t>(static_cast<int>(i.classifier));
		writer.write<uint32_t>(i.id);
		writer.write_c_string(i.name);
		writer.write<uint32_t>(i.is_comment);
		writer.write<uint32_t>(i.open_state);
		writer.write<uint32_t>(i.parent_id);
	}

	for (const auto& i : triggers) {
		writer.write<uint32_t>(static_cast<int>(i.classifier));
		writer.write_c_string(i.name);
		writer.write_c_string(i.description);

		writer.write<uint32_t>(i.is_comment);
		writer.write<uint32_t>(i.id);
		writer.write<uint32_t>(i.is_enabled);
		writer.write<uint32_t>(i.is_script);
		writer.write<uint32_t>(!i.initially_on);
		writer.write<uint32_t>(i.run_on_initialization);
		writer.write<uint32_t>(i.parent_id);
		writer.write<uint32_t>(i.ecas.size());
		for (const auto& eca : i.ecas) {
			print_eca_structure(writer, eca, false);
		}
	}

	for (const auto& i : variables) {
		writer.write<uint32_t>(static_cast<int>(Classifier::variable));
		writer.write<uint32_t>(i.id);
		writer.write_c_string(i.name);
		writer.write<uint32_t>(i.parent_id);
	}

	hierarchy.map_file_write("war3map.wtg", writer.buffer);
}

void Triggers::save_jass() const {
	BinaryWriter writer;

	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(1);

	writer.write_c_string(global_jass_comment);
	if (global_jass.size() == 0) {
		writer.write<uint32_t>(0);
	} else {
		writer.write<uint32_t>(global_jass.size() + (global_jass.back() == '\0' ? 0 : 1));
		writer.write_c_string(global_jass);
	}

	// Custom text (jass) needs to be saved in the order they appear in the hierarchy
	for (const auto& j : categories) {
		for (const auto& i : triggers) {
			if (i.parent_id == j.id) {
				if (!i.is_comment) {
					if (i.custom_text.size() == 0) {
						writer.write<uint32_t>(0);
					} else {
						writer.write<uint32_t>(i.custom_text.size() + (i.custom_text.back() == '\0' ? 0 : 1));
						writer.write_c_string(i.custom_text);
					}
				}
			}
		}
	}

	hierarchy.map_file_write("war3map.wct", writer.buffer);
}

void Triggers::generate_global_variables(MapScriptWriter& script, std::unordered_map<std::string, std::string>& unit_variables, std::unordered_map<std::string, std::string>& destructable_variables) {
	for (const auto& variable : variables) {
		if (variable.is_array) {
			script.write_ln("udg_", variable.name, " = __jarray(\"\")");
		} else {
			script.write_ln("udg_", variable.name, " = nil");
		}
	}

	for (const auto& i : map->regions.regions) {
		std::string region_name = i.name;
		trim(region_name);
		std::replace(region_name.begin(), region_name.end(), ' ', '_');
		script.write_ln("gg_rct_", region_name, " = nil");
	}

	for (const auto& i : map->cameras.cameras) {
		std::string camera_name = i.name;
		trim(camera_name);
		std::replace(camera_name.begin(), camera_name.end(), ' ', '_');
		script.write_ln("gg_cam_", camera_name, " = nil");
	}

	for (const auto& i : map->sounds.sounds) {
		std::string sound_name = i.name;
		trim(sound_name);
		std::replace(sound_name.begin(), sound_name.end(), ' ', '_');
		script.write_ln(sound_name, " = nil");
	}

	for (const auto& i : triggers) {
		if (i.is_comment || !i.is_enabled) {
			continue;
		}

		std::string trigger_name = i.name;
		trim(trigger_name);
		std::replace(trigger_name.begin(), trigger_name.end(), ' ', '_');
		script.write_ln("gg_trg_", trigger_name, " = nil");
	}

	for (const auto& [creation_number, type] : unit_variables) {
		script.write_ln("gg_unit_", type, "_", creation_number, " = nil");
	}

	for (const auto& [creation_number, type] : destructable_variables) {
		script.write_ln("gg_dest_", type, "_", creation_number, " = nil");
	}
}

void Triggers::generate_init_global_variables(MapScriptWriter& script) {
	script.function("InitGlobals", [&]() {
		for (const auto& variable : variables) {
			const std::string base_type = trigger_data.data("TriggerTypes", variable.type, 4);
			const std::string type = base_type.empty() ? variable.type : base_type;
			std::string default_value = trigger_data.data("TriggerTypeDefaults", type);

			if (!variable.is_initialized && default_value.empty()) {
				continue;
			}

			if (variable.is_array) {
				script.forloop(0, variable.array_size, [&]() {
					if (variable.is_initialized) {
						if (type == "string" && variable.initial_value.empty()) {
							script.write_ln("udg_", variable.name, "[i] = \"\"");
						} else {
							script.write_ln("udg_", variable.name, "[i] =\"", variable.initial_value, "\"");
						}
					} else {
						if (type == "string") {
							script.write_ln("udg_", variable.name, "[i] = \"\"");
						} else {
							script.write_ln("udg_", variable.name, "[i] = ", default_value);
						}
					}
				});
			} else if (type == "string") {
				if (variable.is_initialized) {
					script.write_ln("udg_", variable.name, " = \"", variable.initial_value, "\"");
				} else {
					script.write_ln("udg_", variable.name, " = \"\"");
				}
			} else {
				if (variable.is_initialized) {
					std::string converted_value = trigger_data.data("TriggerParams", variable.initial_value, 2);

					if (converted_value.empty()) {
						script.write_ln("udg_", variable.name, " = ", variable.initial_value);
					} else {
						script.write_ln("udg_", variable.name, " = ", converted_value);
					}
				} else {
					script.write_ln("udg_", variable.name, " = ", default_value);
				}
			}
		}
	});
}

void Triggers::generate_units(MapScriptWriter& script, std::unordered_map<std::string, std::string>& unit_variables) {
	script.function("CreateUnits", [&]() {
		script.write("local u\n");
		script.write("local unitID\n");
		script.write("local t\n");
		script.write("local life\n");

		for (const auto& i : map->units.units) {
			if (i.id == "sloc") {
				continue;
			}

			std::string unit_reference = "u";
			if (unit_variables.contains(std::format("{:0>4}", i.creation_number))) {
				unit_reference = std::format("gg_unit_{}_{:0>4}", i.id, i.creation_number);
			}

			script.write(std::format("{} = BlzCreateUnitWithSkin(Player({}), FourCC('{}'), {:.4f}, {:.4f}, {:.4f}, FourCC('{}'))\n",
									unit_reference,
									i.player,
									i.id,
									i.position.x * 128.f + map->terrain.offset.x,
									i.position.y * 128.f + map->terrain.offset.y,
									glm::degrees(i.angle),
									i.skin_id));

			if (i.health != -1) {
				script.write(std::format("life = GetUnitState({}, UNIT_STATE_LIFE)\n", unit_reference));
				script.write(std::format("SetUnitState({}, UNIT_STATE_LIFE, {:.4f}* life)\n", unit_reference, i.health / 100.f));
			}

			if (i.mana != -1) {
				script.write(std::format("SetUnitState({}, UNIT_STATE_MANA, {})\n", unit_reference, i.mana));
			}
			if (i.level != 1) {
				script.write(std::format("SetHeroLevel({}, {}, false)\n", unit_reference, i.level));
			}

			if (i.strength != 0) {
				script.write(std::format("SetHeroStr({}, {}, true)\n", unit_reference, i.strength));
			}

			if (i.agility != 0) {
				script.write(std::format("SetHeroAgi({}, {}, true)\n", unit_reference, i.agility));
			}

			if (i.intelligence != 0) {
				script.write(std::format("SetHeroInt({}, {}, true)\n", unit_reference, i.intelligence));
			}

			float range;
			if (i.target_acquisition != -1.f) {
				if (i.target_acquisition == -2.f) {
					range = 200.f;
				} else {
					range = i.target_acquisition;
				}
				script.write(std::format("SetUnitAcquireRange({}, {})\n", unit_reference, range));
			}

			for (const auto& j : i.abilities) {
				for (size_t k = 0; k < std::get<2>(j); k++) {
					script.write(std::format("SelectHeroSkill({}, FourCC('{}'))\n", unit_reference, std::get<0>(j)));
				}

				if (std::get<1>(j)) {
					std::string order_on = abilities_slk.data("orderon", std::get<0>(j));
					if (order_on.empty()) {
						order_on = abilities_slk.data("order", std::get<0>(j));
					}
					script.write(std::format("IssueImmediateOrder({}, \"{}\")\n", unit_reference, order_on));
				} else {
					std::string order_off = abilities_slk.data("orderoff", std::get<0>(j));
					if (!order_off.empty()) {
						script.write(std::format("IssueImmediateOrder({}, \"{}\")\n", unit_reference, order_off));
					}
				}
			}

			for (const auto& j : i.items) {
				script.write(std::format("UnitAddItemToSlotById({}, FourCC('{}'), {})\n", unit_reference, j.second, j.first));
			}

			if (i.item_sets.size()) {
				script.write("t = CreateTrigger()\n");
				script.write("TriggerRegisterUnitEvent(t, " + unit_reference + ", EVENT_UNIT_DEATH)\n");
				script.write("TriggerRegisterUnitEvent(t, " + unit_reference + ", EVENT_UNIT_CHANGE_OWNER)\n");
				script.write("TriggerAddAction(t, UnitItemDrops_" + std::to_string(i.creation_number) + ")\n");
			}
		}
	});
}

void Triggers::generate_items(MapScriptWriter& script) {
	script.function("CreateItems", [&]() {
		script.write("local itemID\n");
		for (const auto& i : map->units.items) {
			script.write(std::format("BlzCreateItemWithSkin(FourCC('{}'), {:.4f}, {:.4f}, FourCC('{}'))\n", i.id, i.position.x * 128.f + map->terrain.offset.x, i.position.y * 128.f + map->terrain.offset.y, i.id));
		}
	});
}

void Triggers::generate_destructables(MapScriptWriter& script, std::unordered_map<std::string, std::string>& destructable_variables) {
	script.function("CreateDestructables", [&]() {
		script.write("local d\n");
		script.write("local t\n");
		script.write("local life\n");

		for (const auto& i : map->doodads.doodads) {
			std::string id = "d";

			if (destructable_variables.contains(std::to_string(i.creation_number))) {
				id = "gg_dest_" + i.id + "_" + std::to_string(i.creation_number);
			}

			if (id == "d" && i.item_sets.empty() && i.item_table_pointer == -1) {
				continue;
			}

			script.write(id + " = BlzCreateDestructableZWithSkin(FourCC('" +
								i.id + "'), " +
								std::to_string(i.position.x * 128.f + map->terrain.offset.x) + ", " +
								std::to_string(i.position.y * 128.f + map->terrain.offset.y) + ", " +
								std::to_string(i.position.z * 128.f) + ", " +
								std::to_string(glm::degrees(i.angle)) + ", " +
								std::to_string(i.scale.x) + ", " +
								std::to_string(i.variation) + ", FourCC('" +
								i.skin_id + "'))\n");

			if (i.life != 100) {
				script.write("life = GetDestructableLife(" + id + ")\n");
				script.write("SetDestructableLife(" + id + ", " + std::to_string(i.life / 100.f) + " * life)\n");
			}

			if (!i.item_sets.empty()) {
				script.write("t = CreateTrigger()\n");
				script.write("TriggerRegisterDeathEvent(t, " + id + ")\n");
				script.write("TriggerAddAction(t, SaveDyingWidget)\n");
				script.write("TriggerAddAction(t, DoodadItemDrops_" + std::to_string(i.creation_number) + ")\n");
			} else if (i.item_table_pointer != -1) {
				script.write("t = CreateTrigger()\n");
				script.write("TriggerRegisterDeathEvent(t, " + id + ")\n");
				script.write("TriggerAddAction(t, SaveDyingWidget)\n");
				script.write("TriggerAddAction(t, ItemTable_" + std::to_string(i.item_table_pointer) + ")\n");
			}
		}
	});
}

void Triggers::generate_regions(MapScriptWriter& script) {
	script.function("CreateRegions", [&]() {
		script.write("local we\n\n");
		for (const auto& i : map->regions.regions) {
			std::string region_name = "gg_rct_" + i.name;
			trim(region_name);
			std::replace(region_name.begin(), region_name.end(), ' ', '_');

			script.write(std::format("{} = Rect({}, {}, {}, {})\n", region_name, std::min(i.left, i.right), std::min(i.bottom, i.top), std::max(i.left, i.right), std::max(i.bottom, i.top)));

			if (!i.weather_id.empty()) {
				script.write(std::format("we = AddWeatherEffect({}, FourCC('{}'))\n", region_name, i.weather_id));
				script.function_call("EnableWeatherEffect", "we", true);
			}
		}
	});
}

void Triggers::generate_cameras(MapScriptWriter& script) {
	script.function("CreateCameras", [&]() {
		for (const auto& i : map->cameras.cameras) {
			std::string camera_name = "gg_cam_" + i.name;
			trim(camera_name);
			std::replace(camera_name.begin(), camera_name.end(), ' ', '_');

			script.write(camera_name + " = CreateCameraSetup()\n");
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ZOFFSET", i.z_offset, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ROTATION", i.rotation, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ANGLE_OF_ATTACK", i.angle_of_attack, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_TARGET_DISTANCE", i.distance, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_ROLL", i.roll, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_FIELD_OF_VIEW", i.fov, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_FARZ", i.far_z, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_NEARZ", i.near_z, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_PITCH", i.local_pitch, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_YAW", i.local_yaw, 0.0);
			script.function_call("CameraSetupSetField", camera_name, "CAMERA_FIELD_LOCAL_ROLL", i.local_roll, 0.0);

			script.function_call("CameraSetupSetDestPosition", camera_name, i.target_x, i.target_y, 0.0);
		}
	});
}

// Todo, missing fields, soundduration also wrong
void Triggers::generate_sounds(MapScriptWriter& script) {
	script.function("InitSounds", [&]() {
		for (const auto& i : map->sounds.sounds) {
			std::string sound_name = i.name;
			trim(sound_name);
			std::replace(sound_name.begin(), sound_name.end(), ' ', '_');

			script.write(std::format("{} = CreateSound(\"{}\", {}, {}, {}, {}, {}, \"{}\")\n",
											sound_name,
											string_replaced(i.file, "\\", "\\\\"),
											i.looping ? "true" : "false",
											i.is_3d ? "true" : "false",
											i.stop_out_of_range ? "true" : "false",
											i.fade_in_rate,
											i.fade_out_rate,
											string_replaced(i.eax_effect, "\\", "\\\\")));

			script.function_call("SetSoundDuration", sound_name, i.fade_in_rate);
			script.function_call("SetSoundChannel", sound_name, i.channel);
			script.function_call("SetSoundVolume", sound_name, i.volume);
			script.function_call("SetSoundPitch", sound_name, i.pitch);
		}
	});
}

void Triggers::write_item_table_entry(MapScriptWriter& script, int chance, const std::string& id) {
	if (id == "") {
		script.function_call("RandomDistAddItem", -1, chance);
	} else if (id[0] == 'Y' && id[2] == 'I' &&
			   ((id[1] >= 'i' && id[1] <= 'o') || id[1] == 'Y')) { // Random items
		script.write("RandomDistAddItem(ChooseRandomItemEx(ITEM_TYPE_");
		switch (id[1]) {
			case 'i': // permanent
				script.write("PERMANENT, ");
				break;
			case 'j': // charged
				script.write("CHARGED, ");
				break;
			case 'k': // powerup
				script.write("POWERUP, ");
				break;
			case 'l': // artifact
				script.write("ARTIFACT, ");
				break;
			case 'm': // purchasable
				script.write("PURCHASABLE, ");
				break;
			case 'n': // campaign
				script.write("CAMPAIGN, ");
				break;
			case 'o': // miscellaneous
				script.write("MISCELLANEOUS, ");
				break;
			case 'Y': // any category
				script.write("ANY, ");
				break;
		}
		if (id[3] == '/') { // any level
			script.write("-1), ");
		} else {
			script.write(std::string(1, id[3]) + "), ");
		}
		script.write(std::to_string(chance) + ")\n");
	} else {
		script.function_call("RandomDistAddItem", "FourCC('" + id + "')", chance);
	}
}

template<typename T>
void generate_item_tables(MapScriptWriter& script, std::string table_name_prefix, std::vector<T> table_holders) {
	for (const auto& i : table_holders) {
		if (i.item_sets.size()) {
			continue;
		}

		script.function(table_name_prefix + std::to_string(i.creation_number), [&]() {
			script.write("local trigWidget = nil\n");
			script.write("local trigUnit = nil\n");
			script.write("local itemID = 0\n");
			script.write("local canDrop = true\n");
			script.write("trigWidget = bj_lastDyingWidget\n");

			script.write("if (trigWidget == nil) then\n");
			script.write("trigUnit=GetTriggerUnit()\n");
			script.write("end\n");
			script.write("if (trigUnit ~= nil) then\n");
			script.write("canDrop = not IsUnitHidden(trigUnit)\n");
			script.write("if (canDrop and GetChangingUnit() ~= nil) then\n");
			script.write("canDrop = (GetChangingUnitPrevOwner() == Player(PLAYER_NEUTRAL_AGGRESSIVE))\n");
			script.write("end\n");
			script.write("end\n");
			script.write("if (canDrop) then\n");

			for (const auto& j : i.item_sets) {
				script.function_call("RandomDistReset");
				for (const auto& [chance, id] : j.items) {
					Triggers::write_item_table_entry(script, chance, id);
				}

				script.write("itemID = RandomDistChoose()\n");
				script.write("if (trigUnit ~= nil) then\n");
				script.write("UnitDropItem(trigUnit, itemID)\n");
				script.write("else\n");
				script.write("WidgetDropItem(trigWidget, itemID)\n");
				script.write("end\n");
			}

			script.write("end\n");
			script.write("bj_lastDyingWidget = nil\n");
			script.write("DestroyTrigger(GetTriggeringTrigger())\n");
		});
	}
}

void Triggers::generate_trigger_initialization(MapScriptWriter& script, std::vector<std::string> initialization_triggers) {
	script.function("InitCustomTriggers", [&]() {
		for (const auto& i : triggers) {
			if (i.is_comment || !i.is_enabled) {
				continue;
			}
			std::string trigger_name = i.name;
			trim(trigger_name);
			std::replace(trigger_name.begin(), trigger_name.end(), ' ', '_');

			script.function_call("InitTrig_" + trigger_name);
		}
	});

	script.function("RunInitializationTriggers", [&]() {
		for (const auto& i : initialization_triggers) {
			script.function_call("ConditionalTriggerExecute", i);
		}
	});
}

void Triggers::generate_players(MapScriptWriter& script) {
	script.function("InitCustomPlayerSlots", [&]() {
		const std::vector<std::string> players = { "MAP_CONTROL_USER", "MAP_CONTROL_COMPUTER", "MAP_CONTROL_NEUTRAL", "MAP_CONTROL_RESCUABLE" };
		const std::vector<std::string> races = { "RACE_PREF_RANDOM", "RACE_PREF_HUMAN", "RACE_PREF_ORC", "RACE_PREF_UNDEAD", "RACE_PREF_NIGHTELF" };

		size_t index = 0;
		for (const auto& i : map->info.players) {
			std::string player = "Player(" + std::to_string(i.internal_number) + ")";

			script.function_call("SetPlayerStartLocation", player, index);
			if (i.fixed_start_position || i.race == PlayerRace::selectable) {
				script.function_call("ForcePlayerStartLocation", player, index);
			}

			script.function_call("SetPlayerColor", player, "ConvertPlayerColor(" + std::to_string(i.internal_number) + ")");
			script.function_call("SetPlayerRacePreference", player, races[static_cast<int>(i.race)]);
			script.function_call("SetPlayerRaceSelectable", player, true);
			script.function_call("SetPlayerController", player, players[static_cast<int>(i.type)]);

			if (i.type == PlayerType::rescuable) {
				for (const auto& j : map->info.players) {
					if (j.type == PlayerType::human) {
						script.function_call("SetPlayerAlliance", player, "Player(" + std::to_string(j.internal_number) + ")", "ALLIANCE_RESCUABLE", true);
					}
				}
			}

			script.write("\n");
			index++;
		}
	});
}

void Triggers::generate_custom_teams(MapScriptWriter& script) {
	script.function("InitCustomTeams", [&]() {
		int current_force = 0;
		for (const auto& i : map->info.forces) {
			for (const auto& j : map->info.players) {
				if (i.player_masks & (1 << j.internal_number)) {
					script.function_call("SetPlayerTeam", "Player(" + std::to_string(j.internal_number) + ")", current_force);

					if (i.allied_victory) {
						script.function_call("SetPlayerState", "Player(" + std::to_string(j.internal_number) + ")", "PLAYER_STATE_ALLIED_VICTORY", 1);
					}
				}
			}

			for (const auto& j : map->info.players) {
				if (i.player_masks & (1 << j.internal_number)) {
					for (const auto& k : map->info.players) {
						if (i.player_masks & (1 << k.internal_number) && j.internal_number != k.internal_number) {
							if (i.allied) {
								script.function_call("SetPlayerAllianceStateAllyBJ", "Player(" + std::to_string(j.internal_number) + ")",  "Player(" + std::to_string(k.internal_number) + ")", true);
							}
							if (i.share_vision) {
								script.function_call("SetPlayerAllianceStateVisionBJ", "Player(" + std::to_string(j.internal_number) + ")",  "Player(" + std::to_string(k.internal_number) + ")", true);
							}
							if (i.share_unit_control) {
								script.function_call("SetPlayerAllianceStateControlBJ", "Player(" + std::to_string(j.internal_number) + ")",  "Player(" + std::to_string(k.internal_number) + ")", true);
							}
							if (i.share_advanced_unit_control) {
								script.function_call("SetPlayerAllianceStateFullControlBJ", "Player(" + std::to_string(j.internal_number) + ")",  "Player(" + std::to_string(k.internal_number) + ")", true);
							}
						}
					}
				}
			}
			current_force++;
		}
	});
}

void Triggers::generate_ally_priorities(MapScriptWriter& script) {
	script.function("InitAllyPriorities", [&]() {
		std::unordered_map<int, int> player_to_startloc;

		int current_player = 0;
		for (const auto& i : map->info.players) {
			player_to_startloc[i.internal_number] = current_player;
			current_player++;
		}

		current_player = 0;
		for (const auto& i : map->info.players) {
			size_t count = 0;
			for (const auto& j : map->info.players) {
				if (i.ally_low_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					count++;
				} else if (i.ally_high_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					count++;
				}
			}

			script.function_call("SetStartLocPrioCount", current_player, count);

			size_t current_index = 0;
			for (const auto& j : map->info.players) {
				if (i.ally_low_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					script.function_call("SetStartLocPrio", current_player, current_index, player_to_startloc[j.internal_number], "MAP_LOC_PRIO_LOW");
					current_index++;
				} else if (i.ally_high_priorities_flags & (1 << j.internal_number) && i.internal_number != j.internal_number) {
					script.function_call("SetStartLocPrio", current_player, current_index, player_to_startloc[j.internal_number], "MAP_LOC_PRIO_HIGH");
					current_index++;
				}
			}

			current_player++;
		}
	});
}

void Triggers::generate_main(MapScriptWriter& script) {
	script.function("main", [&]() {
		script.function_call("SetCameraBounds",
			std::to_string(map->info.camera_left_bottom.x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT)",
			std::to_string(map->info.camera_left_bottom.y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM)",

			std::to_string(map->info.camera_right_top.x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT)",
			std::to_string(map->info.camera_right_top.y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP)",

			std::to_string(map->info.camera_left_top.x - 512.f) + " + GetCameraMargin(CAMERA_MARGIN_LEFT)",
			std::to_string(map->info.camera_left_top.y + 256.f) + " - GetCameraMargin(CAMERA_MARGIN_TOP)",

			std::to_string(map->info.camera_right_bottom.x + 512.f) + " - GetCameraMargin(CAMERA_MARGIN_RIGHT)",
			std::to_string(map->info.camera_right_bottom.y - 256.f) + " + GetCameraMargin(CAMERA_MARGIN_BOTTOM)"
		);

		const std::string terrain_lights = string_replaced(world_edit_data.data("TerrainLights", ""s + map->terrain.tileset), "\\", "/");
		const std::string unit_lights = string_replaced(world_edit_data.data("TerrainLights", ""s + map->terrain.tileset), "\\", "/");
		script.function_call("SetDayNightModels", "\"" + terrain_lights + "\"", "\"" + unit_lights + "\"");

		const std::string sound_environment = string_replaced(world_edit_data.data("SoundEnvironment", ""s + map->terrain.tileset), "\\", "/");
		script.function_call("NewSoundEnvironment", "\"" + sound_environment + "\"");

		const std::string ambient_day = string_replaced(world_edit_data.data("DayAmbience", ""s + map->terrain.tileset), "\\", "/");
		script.function_call("SetAmbientDaySound", "\"" + ambient_day + "\"");

		const std::string ambient_night = string_replaced(world_edit_data.data("NightAmbience", ""s + map->terrain.tileset), "\\", "/");
		script.function_call("SetAmbientNightSound", "\"" + ambient_night + "\"");

		script.function_call("SetMapMusic", "\"Music\"", true, 0);
		script.function_call("InitSounds");
		script.function_call("CreateRegions");
		script.function_call("CreateCameras");
		script.function_call("CreateDestructables");
		script.function_call("CreateItems");
		script.function_call("CreateUnits");
		script.function_call("InitBlizzard");
		script.function_call("InitGlobals");
		script.function_call("InitCustomTriggers");
		script.function_call("RunInitializationTriggers");
	});
}

void Triggers::generate_map_configuration(MapScriptWriter& script) {
	script.function("config", [&]() {
		script.function_call("SetMapName", "\"" + map->info.name + "\"");
		script.function_call("SetMapDescription", "\"" + map->info.description + "\"");
		script.function_call("SetPlayers", map->info.players.size());
		script.function_call("SetTeams", map->info.forces.size());
		script.function_call("SetGamePlacement", "MAP_PLACEMENT_USE_MAP_SETTINGS");

		script.write("\n");

		for (const auto& i : map->units.units) {
			if (i.id == "sloc") {
				script.function_call("DefineStartLocation", i.player, i.position.x * 128.f + map->terrain.offset.x, i.position.y * 128.f + map->terrain.offset.y);
			}
		}

		script.write("\n");

		script.function_call("InitCustomPlayerSlots");
		if (map->info.custom_forces) {
			script.function_call("InitCustomTeams");
		} else {
			for (const auto& i : map->info.players) {
				script.function_call("SetPlayerSlotAvailable", "Player(" + std::to_string(i.internal_number) + ")", "MAP_CONTROL_USER");
			}

			script.function_call("InitGenericPlayerSlots");
		}
		script.function_call("InitAllyPriorities");
	});
}

QString Triggers::generate_map_script() {
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
			trigger_script += convert_gui_to_jass(i, initialization_triggers);
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

	// Write the results to a buffer
	BinaryWriter writer;

	MapScriptWriter script_writer;

	generate_global_variables(script_writer, unit_variables, destructable_variables);
	generate_init_global_variables(script_writer);
	generate_item_tables(script_writer, "ItemTable_", map->info.random_item_tables);
	generate_item_tables(script_writer, "UnitItemDrops_", map->units.units);
	generate_item_tables(script_writer, "DoodadItemDrops_", map->doodads.doodads);
	generate_sounds(script_writer);

	generate_destructables(script_writer, destructable_variables);
	generate_items(script_writer);
	generate_units(script_writer, unit_variables);
	generate_regions(script_writer);
	generate_cameras(script_writer);

	writer.write_string(global_jass);

	script_writer.write(trigger_script);

	generate_trigger_initialization(script_writer, initialization_triggers);
	generate_players(script_writer);
	generate_custom_teams(script_writer);
	generate_ally_priorities(script_writer);
	generate_main(script_writer);
	generate_map_configuration(script_writer);

	fs::path path = QDir::tempPath().toStdString() + "/input.lua";
	std::ofstream output(path, std::ios::binary);
	output.write((char*)script_writer.script.data(), script_writer.script.size());
	output.close();

	hierarchy.map_file_add(path, "war3map.lua");

	/*QProcess* proc = new QProcess();
	proc->setWorkingDirectory("Data/Tools");
	proc->start("Data/Tools/clijasshelper.exe", { "--scriptonly", "common.j", "blizzard.j", QString::fromStdString(path.string()), "war3map.j" });
	proc->waitForFinished();
	QString result = proc->readAllStandardOutput();

	if (result.contains("Compile error")) {
		QMessageBox::information(nullptr, "vJass output", "There were compilation errors. See the output tab for more information\n" + result.mid(result.indexOf("Compile error")), QMessageBox::StandardButton::Ok);
		return result.mid(result.indexOf("Compile error"));
	} else if (result.contains("compile errors")) {
		QMessageBox::information(nullptr, "vJass output", "There were compilation errors. See the output tab for more information" + result.mid(result.indexOf("compile errors")), QMessageBox::StandardButton::Ok);
		return result.mid(result.indexOf("compile errors."));
	} else {
		hierarchy.map_file_add("Data/Tools/war3map.j", "war3map.j");
		return "Compilation successful";
	}*/
	return "Compilation successful";
}

std::string Triggers::convert_eca_to_jass(const ECA& eca, std::string& pre_actions, const std::string& trigger_name, bool nested) const {
	std::string output;

	if (!eca.enabled) {
		return "";
	}

	if (eca.name == "WaitForCondition") {
		output += "while (true) do\n";
		output += std::format("if (({})) then break end\n", resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)));
		output += "TriggerSleepAction(RMaxBJ(bj_WAIT_FOR_COND_MIN_INTERVAL, " + resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "))\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "ForLoopAMultiple" || eca.name == "ForLoopBMultiple") {
		std::string loop_index = eca.name == "ForLoopAMultiple" ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		std::string loop_index_end = eca.name == "ForLoopAMultiple" ? "bj_forLoopAIndexEnd" : "bj_forLoopBIndexEnd";

		output += loop_index + "=" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)) + "\n";
		output += loop_index_end + "=" + resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "\n";
		output += "while (true) do\n";
		output += std::format("if (({} > {})) then break end\n", loop_index, loop_index_end);
		for (const auto& i : eca.ecas) {
			output += "" + convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		output += loop_index + " = " + loop_index + " + 1\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "ForLoopVarMultiple") {
		std::string variable = resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "integer");

		output += variable + " = " + resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "\n";
		output += "while (true) do\n";
		output += std::format("if (({} > {})) then break end\n", variable, resolve_parameter(eca.parameters[2], trigger_name, pre_actions, get_type(eca.name, 2)));
		for (const auto& i : eca.ecas) {
			output += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		output += variable + " = " + variable + " + 1\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "IfThenElseMultiple") {
		std::string iftext;
		std::string thentext;
		std::string elsetext;

		std::string function_name = generate_function_name(trigger_name);
		iftext += "function " + function_name + "()\n"; // returns boolean

		for (const auto& i : eca.ecas) {
			if (i.type == ECA::Type::condition) {
				iftext += "if (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
				iftext += "return false\n";
				iftext += "end\n";
			} else if (i.type == ECA::Type::action) {
				if (i.group == 1) {
					thentext += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
				} else {
					elsetext += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
				}
			}
		}
		iftext += "return true\n";
		iftext += "end\n";
		pre_actions += iftext;

		return "if (" + function_name + "()) then\n" + thentext + "else\n" + elsetext + "end";
	}

	if (eca.name == "ForForceMultiple" || eca.name == "ForGroupMultiple" || eca.name == "EnumDestructablesInRectAllMultiple" || eca.name == "EnumDestructablesInCircleBJMultiple") {
		std::string script_name = trigger_data.data("TriggerActions", "_" + eca.name + "_ScriptName");

		const std::string function_name = generate_function_name(trigger_name);

		if (eca.name == "EnumDestructablesInCircleBJMultiple") {
			output += script_name + "(" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)) + ", " +
					  resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + ", function " + function_name + ")\n";
		} else {
			output += script_name + "(" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)) + ", function " + function_name + ")\n";
		}

		std::string toto;
		for (const auto& i : eca.ecas) {
			toto += "" + convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		pre_actions += "function " + function_name + "()\n"; // returns nothing
		pre_actions += toto;
		pre_actions += "\nend\n";

		return output;
	}

	if (eca.name == "AndMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::string iftext = "function " + function_name + "()\n"; // returns boolean
		for (const auto& i : eca.ecas) {
			iftext += "if (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
			iftext += "return false\n";
			iftext += "end\n";
		}
		iftext += "return true\n";
		iftext += "end\n";
		pre_actions += iftext;

		return function_name + "()";
	}

	if (eca.name == "OrMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::string iftext = "function " + function_name + "()\n"; // returns boolean
		for (const auto& i : eca.ecas) {
			iftext += "if (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ") then\n";
			iftext += "return true\n";
			iftext += "end\n";
		}
		iftext += "return false\n";
		iftext += "end\n";
		pre_actions += iftext;

		return function_name + "()";
	}

	return testt(trigger_name, eca.name, eca.parameters, pre_actions, !nested);
}

std::string Triggers::testt(const std::string& trigger_name, const std::string& parent_name, const std::vector<TriggerParameter>& parameters, std::string& pre_actions, bool add_call) const {
	std::string output;

	std::string script_name = trigger_data.data("TriggerActions", "_" + parent_name + "_ScriptName");
 
	if (parent_name == "SetVariable") {
		const std::string &type = (*find_if(variables.begin(), variables.end(),
			[parameters](const TriggerVariable& var) {
				return var.name == parameters[0].value;
			}
		)).type;
		const std::string first = resolve_parameter(parameters[0], trigger_name, pre_actions, "");
		const std::string second = resolve_parameter(parameters[1], trigger_name, pre_actions, type);

		return first + " = " + second;
	}

	if (parent_name == "CommentString") {
		return "//" + resolve_parameter(parameters[0], trigger_name, pre_actions, "");
	}

	if (parent_name == "CustomScriptCode") {
		return resolve_parameter(parameters[0], trigger_name, pre_actions, "");
	}

	if (parent_name.substr(0, 15) == "OperatorCompare") {
		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		auto result_operator = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		if (result_operator == "!=") {
			result_operator = "~=";
		}

		output += " " + result_operator + " ";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2));
		return output;
	}

	if (parent_name == "OperatorString") {
		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		output += " .. ";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + ")";
		return output;
	}

	if (parent_name == "ForLoopVar") {
		std::string variable = resolve_parameter(parameters[0], trigger_name, pre_actions, "integer");

		output += variable + " = ";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + "\n";
		
		output += "while (true) do\n";
		output += std::format("if (({} > {})) then break end\n", variable, resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2)));
		output += resolve_parameter(parameters[3], trigger_name, pre_actions, get_type(parent_name, 3), true) + "\n";
		output += variable + " = " + variable + " + 1\n";
		output += "end\n";
		
		return output;
	}

	if (parent_name == "IfThenElse") {
		std::string thentext;
		std::string elsetext;

		std::string function_name = generate_function_name(trigger_name);
		std::string tttt = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		output += "if (" + function_name + "())\n";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1), true) + "\n";
		output += "else\n";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2), true) + "\n";
		output += "end";

		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return " + tttt + "\n";
		pre_actions += "end\n";
		return output;
	}

	if (parent_name == "ForForce" || parent_name == "ForGroup") {
		std::string function_name = generate_function_name(trigger_name);

		std::string tttt = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		output += parent_name + "(";
		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		output += ", " + function_name;
		output += ")";

		pre_actions += "function " + function_name + "()\n"; // returns nothing
		pre_actions += tttt + "\n";
		pre_actions += "end\n\n";
		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "GetBooleanAnd") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		std::string function_name = generate_function_name(trigger_name);
		output += "GetBooleanAnd(" + function_name + "(), ";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + first_parameter + ")\n";
		pre_actions += "end\n\n";

		function_name = generate_function_name(trigger_name);
		output += function_name + "())";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + second_parameter + ")\n";
		pre_actions += "end\n\n";

		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "GetBooleanOr") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		std::string function_name = generate_function_name(trigger_name);
		output += "GetBooleanOr(" + function_name + "(), ";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + first_parameter + ")\n";
		pre_actions += "end\n\n";

		function_name = generate_function_name(trigger_name);
		output += function_name + "())";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + second_parameter + ")\n";
		pre_actions += "end\n\n";

		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "OperatorInt" || parent_name == "OperatorReal") {
		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		auto result_operator = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		if (result_operator == "!=") {
			result_operator = "~=";
		}

		output += " " + result_operator + " ";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2)) + ")";
		return output;
	}

	if (parent_name == "AddTriggerEvent") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		output += second_parameter.insert(second_parameter.find_first_of('(') + 1, first_parameter + ", ");
		return /*(add_call ? "call " : "") + */output;
	}

	for (size_t k = 0; k < parameters.size(); k++) {
		const auto& i = parameters[k];

		const std::string type = get_type(parent_name, k);

		if (type == "boolexpr") {
			const std::string function_name = generate_function_name(trigger_name);

			std::string tttt = resolve_parameter(parameters[k], trigger_name, pre_actions, type);

			pre_actions += "function " + function_name + "()\n"; // returns boolean
			pre_actions += "return " + tttt + "\n";
			pre_actions += "end\n\n";

			output += function_name;
		} else if (type == "code") {
			const std::string function_name = generate_function_name(trigger_name);

			std::string tttt = resolve_parameter(parameters[k], trigger_name, pre_actions, type);

			pre_actions += "function " + function_name + "()\n"; // returns nothing
			pre_actions += tttt + "\n";
			pre_actions += "end\n\n";

			output += function_name;
		} else {
			output += resolve_parameter(i, trigger_name, pre_actions, type);
		}

		if (k < parameters.size() - 1) {
			output += ", ";
		}
	}

	return /*(add_call ? "call " : "") + */(script_name.empty() ? parent_name : script_name) + "(" + output + ")";
}

std::string Triggers::resolve_parameter(const TriggerParameter& parameter, const std::string& trigger_name, std::string& pre_actions, const std::string& type, bool add_call) const {
	if (parameter.has_sub_parameter) {
		return testt(trigger_name, parameter.sub_parameter.name, parameter.sub_parameter.parameters, pre_actions, add_call);
	} else {
		switch (parameter.type) {
			case TriggerParameter::Type::invalid:
				std::print("Invalid parameter type\n");
				return "";
			case TriggerParameter::Type::preset: {
				const std::string preset_type = trigger_data.data("TriggerParams", parameter.value, 1);

				if (get_base_type(preset_type) == "string") {
					return string_replaced(trigger_data.data("TriggerParams", parameter.value, 2), "`", "\"");
				}

				if (preset_type == "timedlifebuffcode" // ToDo this seems like a hack?
					|| type == "abilcode" 
					|| type == "buffcode" 
					|| type == "destructablecode" 
					|| type == "itemcode" 
					|| type == "ordercode" 
					|| type == "techcode" 
					|| type == "unitcode" 
					|| type == "heroskillcode" 
					|| type == "weathereffectcode" 
					|| type == "timedlifebuffcode" 
					|| type == "doodadcode" 
					|| type == "timedlifebuffcode" 
					|| type == "terraintype")
				{
					return "FourCC(" + trigger_data.data("TriggerParams", parameter.value, 2) + ")";
				}

				return trigger_data.data("TriggerParams", parameter.value, 2);
			}
			case TriggerParameter::Type::function:
				return parameter.value + "()";
			case TriggerParameter::Type::variable: {
				std::string output = parameter.value;
				
				if (!output.starts_with("gg_")) {
					output = "udg_" + output;
				}

				if (parameter.is_array) {
					output += "[" + resolve_parameter(parameter.parameters[0], trigger_name, pre_actions, "integer") + "]";
				}
				return output;
			}
			case TriggerParameter::Type::string:
				std::string import_type = trigger_data.data("TriggerTypes", type, 5);

				if (!import_type.empty()) {
					return "\"" + string_replaced(parameter.value, "\\", "\\\\") + "\"";
				} else if (get_base_type(type) == "string") {
					return "\"" + parameter.value + "\"";
				} else if (type == "abilcode" // ToDo this seems like a hack?
					|| type == "buffcode"
					|| type == "destructablecode"
					|| type == "itemcode"
					|| type == "ordercode"
					|| type == "techcode"
					|| type == "unitcode"
					|| type == "heroskillcode"
					|| type == "weathereffectcode" 
					|| type == "timedlifebuffcode"
					|| type == "doodadcode"
					|| type == "timedlifebuffcode"
					|| type == "terraintype") {
					return "FourCC('" + parameter.value + "')";
				} else {
					return parameter.value;
				}
		}
	}
	std::print("Unable to resolve parameter for trigger: {} and parameter value {}\n", trigger_name, parameter.value);
	return "";
}

std::string Triggers::get_base_type(const std::string& type) const {
	std::string base_type = trigger_data.data("TriggerTypes", type, 4);

	if (base_type.empty()) {
		return type;
	}

	return base_type;
}

std::string Triggers::get_type(const std::string& function_name, int parameter) const {
	std::string type;

	if (trigger_data.key_exists("TriggerActions", function_name)) {
		type = trigger_data.data("TriggerActions", function_name, 1 + parameter);
	} else if (trigger_data.key_exists("TriggerCalls", function_name)) {
		type = trigger_data.data("TriggerCalls", function_name, 3 + parameter);
	} else if (trigger_data.key_exists("TriggerEvents", function_name)) {
		type = trigger_data.data("TriggerEvents", function_name, 1 + parameter);
	} else if (trigger_data.key_exists("TriggerConditions", function_name)) {
		type = trigger_data.data("TriggerConditions", function_name, 1 + parameter);
	}
	return type;
}

std::string Triggers::generate_function_name(const std::string& trigger_name) const {
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return "Trig_" + trigger_name + "_" + std::to_string(time & 0xFFFFFFFF);
}

std::string Triggers::convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& map_initializations) const {
	std::string trigger_name = trigger.name;
	trim(trigger_name);
	std::replace(trigger_name.begin(), trigger_name.end(), ' ', '_');

	std::string trigger_variable_name = "gg_trg_" + trigger_name;
	std::string trigger_action_name = "Trig_" + trigger_name + "_Actions";

	std::string events;
	std::string conditions;
	std::string pre_actions;
	std::string actions;

	events += "function InitTrig_" + trigger_name + "()\n";
	events += trigger_variable_name + " = CreateTrigger()\n";

	actions += "function " + trigger_action_name + "()\n";

	for (const auto& i : trigger.ecas) {
		if (!i.enabled) {
			continue;
		}

		switch (i.type) {
			case ECA::Type::event:
				if (i.name == "MapInitializationEvent") {
					map_initializations.push_back(trigger_variable_name);
					continue;
				}
				events += i.name + "(" + trigger_variable_name + ", ";
				for (size_t k = 0; k < i.parameters.size(); k++) {
					const auto& p = i.parameters[k];

					if (get_type(i.name, k) == "VarAsString_Real") {
						events += "\"" + resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k)) + "\"";
					} else {
						events += resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k));
					}

					if (k < i.parameters.size() - 1) {
						events += ", ";
					}
				}
				events += ")\n";

				break;
			case ECA::Type::condition:
				conditions += "if (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
				conditions += "return false\n";
				conditions += "end\n";
				break;
			case ECA::Type::action:
				actions += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
				break;
		}
	}

	actions += "end\n\n";

	if (!conditions.empty()) {
		conditions = "function Trig_" + trigger_name + "_Conditions()\n" + conditions;
		conditions += "return true\n";
		conditions += "end\n\n";

		events += "TriggerAddCondition(" + trigger_variable_name + ", Condition(Trig_" + trigger_name + "_Conditions))\n";
	}

	if (!trigger.initially_on) {
		events += "DisableTrigger(" + trigger_variable_name + ")\n";
	}
	events += "TriggerAddAction(" + trigger_variable_name + ", " + trigger_action_name + ")\n";
	events += "end\n\n";

	//return separator + "// Trigger: " + trigger_name + "\n" + separator + pre_actions + conditions + actions + separator + events;
	return pre_actions + conditions + actions + events;
}