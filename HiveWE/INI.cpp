#include "stdafx.h"

namespace ini {
	INI::INI(fs::path path) {
		load(path);
	}
	
	void INI::load(fs::path path) {
		std::stringstream file;
		file << hierarchy.open_file(path).buffer.data();

		std::string line;
		std::string current_section = "";
		while (std::getline(file, line)) {
			// Normaly ini files use ; for comments, but Blizzard uses //
			if (line.substr(0, 2) == "//" || line.size() == 0 || line.front() == ';') {
				continue;
			}

			if (line.front() == '[') {
				std::string key = line.substr(1, line.size() - 3);
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

	std::map<std::string, std::string> INI::section(std::string section) {
		if (ini_data.count(section)) {
			return ini_data[section];
		} else {
			return {};
		}
	}

	std::string INI::data(std::string section, std::string key) {
		if (ini_data.count(section) && ini_data[section].count(key)) {
			return ini_data[section][key];
		} else {
			return "";
		}
	}
}