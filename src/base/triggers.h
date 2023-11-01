#pragma once

#include <QString>
#include <unordered_map>
#include <map>
#include <format>

import BinaryReader;
import BinaryWriter;
import INI;

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

struct Trigger {
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

/// A minimal utility wrapper around an std::string that manages newlines, indentation and closing braces
struct MapScriptWriter {
	std::string script;
	size_t current_indentation = 0;

	void raw_write_to_log(std::string_view users_fmt, std::format_args&& args) {
		std::vformat_to(std::back_inserter(script), users_fmt, args);
	}

	template <typename... Args>
	constexpr void function_call(std::string_view name, Args&&... args) {
		std::string work = "{}(";

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

	template <typename... Args>
	constexpr void write_ln(Args&&... args) {
		std::string work = "";
		for (size_t i = 0; i < current_indentation; i++) {
			work += '\t';
		}
		for (size_t i = 0; i < sizeof...(args); i++) {
			work += "{}";
		}
		work.push_back('\n');

		// Reduce binary code size by having only one instantiation
		raw_write_to_log(work, std::make_format_args(args...));
	}

	template <typename T>
	constexpr void forloop(size_t start, size_t end, T callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
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

	 template <typename T>
	 void function(std::string_view name, T callback) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}

		std::format_to(std::back_inserter(script), "function {}()\n", name);
		current_indentation += 1;
		callback();
		current_indentation -= 1;
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}
		script += "end\n";
	 }

	 template <typename T>
	 void global_variable(std::string_view name, T value) {
		for (size_t i = 0; i < current_indentation; i++) {
			script += '\t';
		}
		std::format_to(std::back_inserter(script), "udg_{} = {}", name, value);
	 }
};


class Triggers {
	std::unordered_map<std::string, int> argument_counts;
	const std::string separator = "//===========================================================================\n";

	static constexpr int write_version = 0x80000004;
	static constexpr int write_sub_version = 7;
	static constexpr int write_string_version = 1;

	int unknown1 = 0;
	int unknown2 = 0;
	int trig_def_ver = 2;

	void parse_parameter_structure(BinaryReader& reader, TriggerParameter& parameter, uint32_t version);
	void parse_eca_structure(BinaryReader& reader, ECA& eca, bool is_child, uint32_t version);

	void print_parameter_structure(BinaryWriter& writer, const TriggerParameter& parameter) const;
	void print_eca_structure(BinaryWriter& writer, const ECA& eca, bool is_child) const;

	std::string convert_eca_to_jass(const ECA& lines, std::string& pre_actions, const std::string& trigger_name, bool nested) const;
	std::string testt(const std::string& trigger_name, const std::string& parent_name, const std::vector<TriggerParameter>& parameters, std::string& pre_actions, bool add_call) const;
	std::string resolve_parameter(const TriggerParameter& parameter, const std::string& trigger_name, std::string& pre_actions, const std::string& base_type, bool add_call = false) const;
	std::string get_base_type(const std::string& type) const;
	std::string get_type(const std::string& function_name, int parameter) const;
	std::string generate_function_name(const std::string& trigger_name) const;
	std::string convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& initialization_triggers) const;

	void generate_global_variables(MapScriptWriter& script, std::unordered_map<std::string, std::string>& unit_variables, std::unordered_map<std::string, std::string>& destructable_variables);
	void generate_init_global_variables(MapScriptWriter& script);
	void generate_units(MapScriptWriter& script, std::unordered_map<std::string, std::string>& unit_variables);
	void generate_items(MapScriptWriter& script);
	void generate_destructables(MapScriptWriter& script, std::unordered_map<std::string, std::string>& destructable_variables);
	void generate_regions(MapScriptWriter& script);
	void generate_cameras(MapScriptWriter& script);
	void generate_sounds(MapScriptWriter& script);
	void generate_trigger_initialization(MapScriptWriter& script, std::vector<std::string> initialization_triggers);
	void generate_players(MapScriptWriter& script);
	void generate_custom_teams(MapScriptWriter& script);
	void generate_ally_priorities(MapScriptWriter& script);
	void generate_main(MapScriptWriter& script);
	void generate_map_configuration(MapScriptWriter& script);

  public:
	ini::INI trigger_strings;
	ini::INI trigger_data;

	std::string global_jass_comment;
	std::string global_jass;

	std::vector<TriggerCategory> categories;
	std::vector<TriggerVariable> variables;
	std::vector<Trigger> triggers;

	void load();
	void load_version_31(BinaryReader& reader, uint32_t version);
	void load_version_pre31(BinaryReader& reader, uint32_t version);
	void load_jass();
	void save() const;
	void save_jass() const;
	static void write_item_table_entry(MapScriptWriter& script, int chance, const std::string& id);

	// Returns compile output which could contain errors or general information
	QString generate_map_script();
};