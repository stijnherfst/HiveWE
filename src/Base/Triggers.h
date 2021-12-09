#pragma once

#include <QString>

#include <unordered_map>

#include "INI.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

enum class Classifier
{
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

class Triggers {
	std::unordered_map<std::string, int> argument_counts;
	const std::string separator = "//===========================================================================\n";

	static constexpr int write_version = 0x80000004;
	static constexpr int write_sub_version = 7;
	static constexpr int write_string_version = 1;

	int map_count = 0;
	int library_count = 0;
	int category_count = 0;
	int trigger_count = 0;
	int comment_count = 0;
	int script_count = 0;
	int variable_count = 0;

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

	void generate_global_variables(BinaryWriter& writer, std::unordered_map<std::string, std::string>& unit_variables, std::unordered_map<std::string, std::string>& destructable_variables);
	void generate_init_global_variables(BinaryWriter& writer);
	void generate_units(BinaryWriter& writer, std::unordered_map<std::string, std::string>& unit_variables);
	void generate_items(BinaryWriter& writer);
	void generate_destructables(BinaryWriter& writer, std::unordered_map<std::string, std::string>& destructable_variables);
	void generate_regions(BinaryWriter& writer);
	void generate_cameras(BinaryWriter& writer);
	void generate_sounds(BinaryWriter& writer);
	void generate_item_tables(BinaryWriter& writer);
	void generate_unit_item_tables(BinaryWriter& writer);
	void generate_trigger_initialization(BinaryWriter& writer, std::vector<std::string> initialization_triggers);
	void generate_players(BinaryWriter& writer);
	void generate_custom_teams(BinaryWriter& writer);
	void generate_ally_priorities(BinaryWriter& writer);
	void generate_main(BinaryWriter& writer);
	void generate_map_configuration(BinaryWriter& writer);
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

	// Returns compile output which could contain errors or general information
	QString generate_map_script();
};