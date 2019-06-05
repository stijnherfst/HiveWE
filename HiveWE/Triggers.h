#pragma once

struct TriggerCategory {
	int id;
	std::string name;
	bool is_comment;
};

struct TriggerVariable {
	//	std::string name;
	std::string type;
	bool is_array;
	int array_size = 0;
	bool is_initialized;
	std::string initial_value;
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
	int id;
	std::string name;
	std::string description;
	bool is_comment;
	bool is_enabled;
	std::string custom_text;
	bool initally_off;
	bool run_on_initialization;
	int category_id;
	std::vector<ECA> lines;
};

class Triggers {
	std::unordered_map<std::string, int> argument_counts;
	const std::string seperator = "//===========================================================================\n";

	static constexpr int write_version = 8;
	static constexpr int write_string_version = 1;

	std::string convert_eca_to_jass(const ECA& lines, std::string& pre_actions, const std::string& trigger_name, bool nested) const;
	std::string testt(const std::string& trigger_name, const std::string& parent_name, const std::vector<TriggerParameter>& parameters, std::string& pre_actions, bool add_call) const;
	std::string resolve_parameter(const TriggerParameter& parameter, const std::string& trigger_name, std::string& pre_actions, const std::string& base_type, bool add_call = false) const;
	std::string get_base_type(const std::string& type) const;
	std::string get_type(const std::string& function_name, int parameter) const;
	std::string generate_function_name(const std::string& trigger_name) const;
	std::string convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& initialization_triggers) const;
public:
	ini::INI trigger_strings;
	ini::INI trigger_data;

	std::string global_jass_comment;
	std::string global_jass;

	std::vector<TriggerCategory> categories;
	//std::vector<TriggerVariable> variables;
	std::unordered_map<std::string, TriggerVariable> variables;
	std::vector<Trigger> triggers;

	void load(BinaryReader& reader);
	void load_jass(BinaryReader& reader);
	void save() const;
	void save_jass() const;

	void generate_map_script();

	int next_id = 0;
};