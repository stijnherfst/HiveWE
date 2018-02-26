#include "stdafx.h"

namespace ini {
	INI::INI(const fs::path& path) {
		load(path);
	}
	
	void INI::load(const fs::path& path) {
		std::stringstream file;
		file << hierarchy.open_file(path).buffer.data();

		std::string line;
		std::string current_section;
		while (std::getline(file, line)) {
			// Normaly ini files use ; for comments, but Blizzard uses //
			if (line.substr(0, 2) == "//" || line.empty() || line.front() == ';') {
				continue;
			}

			if (line.front() == '[') {
				const std::string key = line.substr(1, line.size() - 3);
				// If the segment already exists
				if (ini_data.count(key)) {
					continue;
				}
				ini_data[key] = std::map<std::string, std::string>();
				current_section = key;
			} else {
				auto parts = split(line, '=');
				// Key=Content so atleast size 2 to be valid
				if (parts.size() < 2) {
					continue;
				}

				ini_data[current_section][parts[0]] = parts[1];
			}
		}
	}

	/// Replaces all values (not keys) which match one of the keys in substitution INI
	void INI::substitute(const INI& ini, const std::string& section) {
		for (auto&& [section_key, section_value] : ini_data) {
			for (auto&& [key, value] : section_value) {
				for (auto&& part : split(value, ',')) {
					std::string westring = ini.data(section, part);
					if (!westring.empty()) {
						ini_data[section_key][key] = westring;
					}
				}
			}
		}
	}

	std::map<std::string, std::string> INI::section(const std::string& section) const {
		if (ini_data.count(section)) {
			return ini_data.at(section);
		} else {
			return {};
		}
	}

	std::string INI::data(const std::string& section, const std::string& key) const {
		if (ini_data.count(section) && ini_data.at(section).count(key)) {
			return ini_data.at(section).at(key);
		} else {
			return "";
		}
	}
}