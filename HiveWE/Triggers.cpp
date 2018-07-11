#include "stdafx.h"

void Triggers::load(BinaryReader& reader) {
	ini::INI trigger_data = ini::INI("UI/TriggerData.txt");

	for (auto&& section : { "TriggerActions"s, "TriggerEvents"s, "TriggerConditions"s, "TriggerCalls"s }) {
		for (auto&&[key, value] : trigger_data.section(section)) {
			if (key.front() == '_') {
				continue;
			}

			int arguments = 0;
			for (auto&& j : split(value, ',')) {
				arguments += !j.empty() && !is_number(j) && j != "nothing";
			}

			if (section == "TriggerCalls") {
				--arguments;
			}

			argument_counts[key] = arguments;
		}
	}
	
	std::string magic_number = reader.read_string(4);
	if (magic_number != "WTG!") {
		std::cout << "Unknown magic number for war3map.wtg " << magic_number << "\n";
		return;
	}

	int version = reader.read<uint32_t>();

	categories.resize(reader.read<uint32_t>());
	for (auto&& i : categories) {
		i.id = reader.read<uint32_t>();
		i.name = reader.read_c_string();
		if (version == 7) {
			i.is_comment = reader.read<uint32_t>();
		}
	}

	reader.advance(4); // Unknown always 0

	variables.resize(reader.read<uint32_t>());
	for (auto&& i : variables) {
		i.name = reader.read_c_string();
		i.type = reader.read_c_string();
		reader.advance(4); // Unknown always 1
		i.is_array = reader.read<uint32_t>();
		if (version == 7) {
			i.array_size = reader.read<uint32_t>();
		}
		i.is_initialized = reader.read<uint32_t>();
		i.initial_value = reader.read_c_string();
	}

	std::function<void(TriggerParameter&)> parse_parameter_structure = [&](TriggerParameter& parameter) {
		parameter.type = reader.read<uint32_t>();
		parameter.value = reader.read_c_string();
		bool has_sub_parameter = reader.read<uint32_t>();
		if (has_sub_parameter) {
			parameter.sub_parameter.type = reader.read<uint32_t>();
			parameter.sub_parameter.name = reader.read_c_string();
			parameter.sub_parameter.begin_parameters = reader.read<uint32_t>();
			if (parameter.sub_parameter.begin_parameters) {
				parameter.sub_parameter.parameters.resize(argument_counts[parameter.sub_parameter.name]);
				for (auto&& i : parameter.sub_parameter.parameters) {
					parse_parameter_structure(i);
				}
			}
		}
		if (version == 4) {
			if (parameter.type == 2) {
				reader.advance(4); // Unknown always 0
			} else {
				parameter.is_array = reader.read<uint32_t>();
			}
		} else {
			if (has_sub_parameter) {
				reader.advance(4); // Unknown always 0
			}
			parameter.is_array = reader.read<uint32_t>();
		}
		if (parameter.is_array) {
			parameter.parameters.resize(1);
			parse_parameter_structure(parameter.parameters.front());
		}
	};

	
	std::function<void(ECA&, bool)> parse_eca_structure = [&](ECA& eca, bool is_child) {
		eca.type = reader.read<uint32_t>();
		if (is_child) {
			eca.group = reader.read<uint32_t>();
		}
		eca.name = reader.read_c_string();
		eca.enabled = reader.read<uint32_t>();
		eca.parameters.resize(argument_counts[eca.name]);
		for (auto&& i : eca.parameters) {
			parse_parameter_structure(i);
		}
		if (version == 7) {
			eca.ecas.resize(reader.read<uint32_t>());
			for (auto&& i : eca.ecas) {
				parse_eca_structure(i, true);
			}
		}
	};

	triggers.resize(reader.read<uint32_t>());
	for (auto&& i : triggers) {
		i.id = next_id++;
		i.name = reader.read_c_string();
		i.description = reader.read_c_string();
		if (version == 7) {
			i.is_comment = reader.read<uint32_t>();
		}
		i.is_enabled = reader.read<uint32_t>();
		reader.advance(4); // is_custom
		i.initally_off = reader.read<uint32_t>();
		i.run_on_initialization = reader.read<uint32_t>();
		i.category_id = reader.read<uint32_t>();
		i.lines.resize(reader.read<uint32_t>());
		for (auto&& j : i.lines) {
			parse_eca_structure(j, false);
		}
	}
}

void Triggers::load_jass(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();

	if (version == 1) {
		global_jass_comment = reader.read_c_string();
		global_jass = reader.read_string(reader.read<uint32_t>());
	}

	reader.advance(4);
	for (auto&& i : triggers) {
		const int size = reader.read<uint32_t>();
		if (size > 0) {
			i.custom_text = reader.read_string(size);
		}
	}
}

void Triggers::save() const {

}

void Triggers::save_jass() const {

}