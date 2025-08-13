export module Triggers;

import std;
import Hierarchy;
import Utilities;
import Globals;
import <glm/glm.hpp>;
import Units;
import Doodads;
import Regions;
import GameCameras;
import Sounds;
import Terrain;
import MapInfo;
import BinaryReader;
import BinaryWriter;
import INI;

namespace fs = std::filesystem;
using namespace std::literals::string_literals;

export enum class ScriptMode {
	lua,
	jass
};

/// A minimal utility wrapper around a std::string that manages newlines, indentation and closing braces
struct MapScriptWriter {
	std::string script;
	size_t current_indentation = 0;

	ScriptMode mode;

	explicit MapScriptWriter(ScriptMode mode = ScriptMode::lua) : mode(mode) {}

	bool is_empty() {
		return script.empty();
	}

	void merge(const MapScriptWriter& writer) {
		script += writer.script;
	}

	void raw_write_to_log(std::string_view users_fmt, std::format_args&& args) {
		std::vformat_to(std::back_inserter(script), users_fmt, args);
	}

	constexpr void local(std::string_view type, std::string_view name, std::string_view value) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			std::format_to(std::back_inserter(script), "local {} = {}\n", name, value);
		} else {
			std::format_to(std::back_inserter(script), "local {} {} = {}\n", type, name, value);
		}
	}

	constexpr void global(std::string_view type, std::string_view name, std::string_view value) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			std::format_to(std::back_inserter(script), "{} = {}\n", name, value);
		} else {
			std::format_to(std::back_inserter(script), "{} {} = {}\n", type, name, value);
		}
	}

	constexpr void set_variable(std::string_view name, std::string_view value) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			std::format_to(std::back_inserter(script), "{} = {}\n", name, value);
		} else {
			std::format_to(std::back_inserter(script), "set {} = {}\n", name, value);
		}
	}

	template<typename... Args>
	constexpr void inline_call(std::string_view name, Args&&... args) {
		std::string work = "{}(";

		for (size_t i = 0; i < sizeof...(args); i++) {
			work += "{}";
			if (i < sizeof...(args) - 1) {
				work += ", ";
			}
		}
		work += ")";
		// Reduce binary code size by having only one instantiation
		raw_write_to_log(work, std::make_format_args(name, args...));
	}

	template<typename... Args>
	constexpr void call(std::string_view name, Args&&... args) {
		std::string work;

		if (mode == ScriptMode::jass) {
			work = "call {}(";
		} else {
			work = "{}(";
		}

		for (size_t i = 0; i < sizeof...(args); i++) {
			work += "{}";
			if (i < sizeof...(args) - 1) {
				work += ", ";
			}
		}
		work += ")\n";

		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}
		// Reduce binary code size by having only one instantiation
		raw_write_to_log(work, std::make_format_args(name, args...));
	}

	void write(std::string_view string) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		script += string;
	}

	template<typename... Args>
	constexpr void write_ln(Args&&... args) {
		std::string work = std::string(current_indentation, '\t');
		for (size_t i = 0; i < sizeof...(args); i++) {
			work += "{}";
		}
		work.push_back('\n');

		// Reduce binary code size by having only one instantiation
		raw_write_to_log(work, std::make_format_args(args...));
	}

	template<typename T>
	constexpr void forloop(size_t start, size_t end, T callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			//script += '\t';
		}
		std::format_to(std::back_inserter(script), "for i={},{} do\n", start, end);

		current_indentation += 1;
		callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}
		script += "end\n";
	}

	template<typename T>
	void function(std::string_view name, T callback, const std::string_view return_type = "takes nothing returns nothing") {
		for (size_t i = 0; i < current_indentation; i++) {
			// script += '\t';
		}

		if (mode == ScriptMode::lua) {
			std::format_to(std::back_inserter(script), "function {}()\n", name);
		} else {
			std::format_to(std::back_inserter(script), "function {} {}\n", name, return_type);
		}

		current_indentation += 1;
		callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			// script += '\t';
		}

		if (mode == ScriptMode::lua) {
			script += "end\n\n";
		} else {
			script += "endfunction\n\n";
		}
	}

	template<typename T>
	void while_statement(std::string_view condition, T callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		std::format_to(std::back_inserter(script), "while ({}) do\n", condition);

		current_indentation += 1;
		callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			script += "end\n";
		} else {
			// TODO while loops in jass??
			script += "end\n";
		}
	}

	template<typename T>
	void if_statement(std::string_view condition, T callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		std::format_to(std::back_inserter(script), "if ({}) then\n", condition);

		current_indentation += 1;
		callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			script += "end\n";
		} else {
			script += "endif\n";
		}
	}

	template<typename T1, typename T2>
	void if_else_statement(std::string_view condition, T1 if_callback, T2 else_callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		std::format_to(std::back_inserter(script), "if ({}) then\n", condition);

		current_indentation += 1;
		if_callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		script += "else\n";

		current_indentation += 1;
		else_callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		if (mode == ScriptMode::lua) {
			script += "end\n";
		} else {
			script += "endif\n";
		}
	}

	template<typename T>
	void global_variable(std::string_view name, T value) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}
		std::format_to(std::back_inserter(script), "udg_{} = {}", name, value);
	}

	/// The ID should not be quoted
	std::string four_cc(std::string_view id) {
		if (mode == ScriptMode::lua) {
			return std::format("FourCC(\"{}\")", id);
		} else {
			return std::format("\"{}\"", id);
		}
	}

	std::string null() {
		if (mode == ScriptMode::lua) {
			return "nil";
		} else {
			return "null";
		}
	}
};

enum class Classifier {
	map = 1,
	library = 2,
	category = 4,
	gui = 8,
	comment = 16,
	script = 32,
	variable = 64
};

struct TriggerCategory {
	Classifier classifier;
	int id;
	std::string name;
	bool open_state = true;
	bool is_comment = false;
	int parent_id;
};

struct TriggerParameter;

struct TriggerSubParameter {
	enum class Type {
		events,
		conditions,
		actions,
		calls
	};
	Type type;
	std::string name;
	bool begin_parameters;
	std::vector<TriggerParameter> parameters;
};

struct TriggerParameter {
	enum class Type {
		invalid = -1,
		preset,
		variable,
		function,
		string
	};
	Type type;
	int unknown;
	std::string value;
	bool has_sub_parameter;
	TriggerSubParameter sub_parameter;
	bool is_array = false;
	std::vector<TriggerParameter> parameters; // There is really only one so unique_ptr I guess
};

struct ECA {
	enum class Type {
		event,
		condition,
		action
	};

	Type type;
	int group;
	std::string name;
	bool enabled;
	std::vector<TriggerParameter> parameters;
	std::vector<ECA> ecas;
};

export struct Trigger {
	Classifier classifier;
	int id;
	int parent_id = 0;
	std::string name;
	std::string description;
	std::string custom_text;
	bool is_comment = false;
	bool is_enabled = true;
	bool is_script = false;
	bool initially_on = true;
	bool run_on_initialization = false;
	std::vector<ECA> ecas;

	static inline int next_id = 0;
};

struct TriggerVariable {
	std::string name;
	std::string type;
	uint32_t unknown;
	bool is_array;
	int array_size = 0;
	bool is_initialized;
	std::string initial_value;
	int id;
	int parent_id;
};

std::string get_base_type(const std::string& type, const ini::INI& trigger_data) {
	std::string base_type = trigger_data.data("TriggerTypes", type, 4);

	if (base_type.empty()) {
		return type;
	}

	return base_type;
}

export class Triggers {
	std::unordered_map<std::string, int> argument_counts;

	static constexpr int write_version = 0x80000004;
	static constexpr int write_sub_version = 7;
	static constexpr int write_string_version = 1;

	int unknown1 = 0;
	int unknown2 = 0;
	int trig_def_ver = 2;

  public:
	ini::INI trigger_strings;
	ini::INI trigger_data;

	std::string global_jass_comment;
	std::string global_jass;

	std::vector<TriggerCategory> categories;
	std::vector<TriggerVariable> variables;
	std::vector<Trigger> triggers;

  private:
	std::string get_type(const std::string& function_name, int parameter) const;

	std::string
	convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& map_initializations, ScriptMode mode) const;

	std::string resolve_parameter(
		const TriggerParameter& parameter,
		const std::string& trigger_name,
		MapScriptWriter& pre_actions,
		const std::string& type,
		ScriptMode mode
	) const;

	std::string convert_toplevel_eca_to_script(
		const ECA& eca,
		MapScriptWriter& pre_actions,
		const std::string& trigger_name,
		ScriptMode mode
	) const;

	std::string convert_sub_eca_to_script(
		const std::string& trigger_name,
		const std::string& parent_name,
		const std::vector<TriggerParameter>& parameters,
		MapScriptWriter& pre_actions,
		ScriptMode mode
	) const;

	void parse_parameter_structure(BinaryReader& reader, TriggerParameter& parameter, uint32_t version) {
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

	void parse_eca_structure(BinaryReader& reader, ECA& eca, bool is_child, uint32_t version) {
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

	void print_parameter_structure(BinaryWriter& writer, const TriggerParameter& parameter) const {
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

	void print_eca_structure(BinaryWriter& writer, const ECA& eca, bool is_child) const {
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

  public:
	void load() {
		BinaryReader reader = hierarchy.map_file_read("war3map.wtg");

		trigger_strings.load("UI/TriggerStrings.txt");
		trigger_data.load("UI/TriggerData.txt");
		trigger_data.substitute(world_edit_strings, "WorldEditStrings");

		// Manual fixes
		trigger_data.set_whole_data("TriggerTypeDefaults", "string", "\"\"");

		for (auto&& section : {"TriggerActions"s, "TriggerEvents"s, "TriggerConditions"s, "TriggerCalls"s}) {
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

		const std::string magic_number = reader.read_string(4);
		if (magic_number != "WTG!") {
			std::println("Unknown magic number for war3map.wtg {}", magic_number);
			return;
		}

		const uint32_t version = reader.read<uint32_t>();
		if (version == 0x80000004)
			load_version_31(reader, version);
		else if (version == 4 || version == 7)
			load_version_pre31(reader, version);
		else {
			std::println("Unknown WTG format! Trying 1.31 loader");
			load_version_31(reader, version);
		}
	}

	void load_version_31(BinaryReader& reader, uint32_t version) {
		uint32_t sub_version = reader.read<uint32_t>();
		if (sub_version != 7 && sub_version != 4) {
			std::print("Unknown 1.31 WTG subformat! Trying anyway.\n");
		}

		reader.advance(4); // map_count
		reader.advance(4 * reader.read<uint32_t>()); // map ids of deleted maps

		reader.advance(4); // library_count
		reader.advance(4 * reader.read<uint32_t>()); // library ids of deleted libraries

		reader.advance(4); // category_count
		reader.advance(4 * reader.read<uint32_t>()); // category ids of deleted categories

		reader.advance(4); // trigger_count
		reader.advance(4 * reader.read<uint32_t>()); // trigger ids of deleted triggers

		reader.advance(4); // comment_count
		reader.advance(4 * reader.read<uint32_t>()); // comment ids of deleted comments

		reader.advance(4); // script_count
		reader.advance(4 * reader.read<uint32_t>()); // script ids of deleted scripts

		reader.advance(4); // variable_count
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
					reader.advance(4); // id
					reader.advance_c_string(); // name
					reader.advance(4); // parentid
					break;
				}
			}
		}
	}

	void load_version_pre31(BinaryReader& reader, uint32_t version) {
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
		categories.insert(categories.begin(), {Classifier::map, 0, "Map Header", true, false, -1});
		categories.insert(categories.begin(), {Classifier::category, variable_category, "Variables", true, false, 0});

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

	void load_jass() {
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

	void save() const {
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

	void save_jass() const {
		BinaryWriter writer;

		writer.write<uint32_t>(write_version);
		writer.write<uint32_t>(1);

		writer.write_c_string(global_jass_comment);
		if (global_jass.empty()) {
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

	// Returns compile output which could contain errors or general information
	std::string generate_map_script(
		const Terrain& terrain,
		const Units& units,
		const Doodads& doodads,
		const MapInfo& map_info,
		const Sounds& sounds,
		const Regions& regions,
		const GameCameras& cameras,
		ScriptMode mode
	);
};