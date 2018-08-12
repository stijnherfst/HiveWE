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
				std::string key = line.substr(1, line.size() - 3);

				if (key.back() == ']') {
					key.pop_back();
				}
				// If the segment already exists
				if (ini_data.count(key)) { // ToDo C++20 contains
					continue;
				}
				ini_data[key] = std::map<std::string, std::vector<std::string>>();
				current_section = key;
			} else {
				auto found = line.find_first_of('=');
				if (found == std::string::npos) {
					continue;
				}

				std::string key = line.substr(0, found);
				std::string value = line.substr(found + 1, line.size() - found - 2); // Strip \r
				auto parts = split(value, ',');
				// Strip of quotes at the front/back
				for (auto&& i : parts) {
					if (i.size() < 2) {
						continue;
					}
					if (i.front() == '\"') {
						i.erase(i.begin());
					}
					if (i.back() == '\"') {
						i.erase(i.end() - 1);
					}
				}
				ini_data[current_section][key] = parts;
			}
		}
	}

	/// Replaces all values (not keys) which match one of the keys in substitution INI
	void INI::substitute(const INI& ini, const std::string& section) {
		for (auto&& [section_key, section_value] : ini_data) {
			for (auto&& [key, value] : section_value) {
				for (auto&& part : value) {
					std::string westring = ini.data(section, part);
					if (!westring.empty()) {
						part = westring;
					}
				}
			}
		}
	}

	std::map<std::string, std::vector<std::string>> INI::section(const std::string& section) const {
		if (ini_data.count(section)) {
			return ini_data.at(section);
		} else {
			return {};
		}
	}

	std::string INI::data(const std::string& section, const std::string& key, const int argument) const {
		if (ini_data.count(section) && ini_data.at(section).count(key) && argument < ini_data.at(section).at(key).size()) { // ToDo C++20 contains
			return ini_data.at(section).at(key)[argument];
		} else {
			return "";
		}
	}

	std::vector<std::string> INI::whole_data(const std::string& section, const std::string& key) const {
		if (ini_data.count(section) && ini_data.at(section).count(key)) { // ToDo C++20 contains
			return ini_data.at(section).at(key);
		} else {
			return {};
		}
	}
}