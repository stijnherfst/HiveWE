module;

#include <string>

export module MapScriptGenerator;

export class MapScriptGenerator {
	std::string get_base_type(const std::string& type) const {
		std::string base_type = trigger_data.data("TriggerTypes", type, 4);

		if (base_type.empty()) {
			return type;
		}

		return base_type;
	}
};