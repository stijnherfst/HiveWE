#include "stdafx.h"

namespace json {
	JSON::JSON(const fs::path& path) {
		load(path);
	}

	void JSON::load(const fs::path& path) {
		json_data.clear();
		std::stringstream file;
		file << hierarchy.open_file(path).buffer.data();
		size_t end1;
		std::string line;
		if (std::getline(file, line)) {
			if (line.front() == '[') {
				while (std::getline(file, line) && line.back() != '}') {
					// Normaly json files use ; for comments, but Blizzard uses //
					if (line.substr(0, 2) == "//" || line.empty() || line.front() == ';') {
						continue;
					}
					for (int i = 0; i < line.length(); i++) {
						if (line.at(i) == '/') {
							line.replace(i, 1, "\\");
						}
					}
					if (line.substr(0, 12) == "    {\"src\":\"") {
						end1 = line.find('\"', 13);
						std::string key = line.substr(12, end1 - 12);
						std::transform(key.begin(), key.end(), key.begin(), ::tolower);
						// If the segment already exists
						if (json_data.contains(key)) {
							continue;
						}
						json_data[key] = line.substr(end1 + 11, line.find('\"', end1 + 12) - end1 - 11);
					}
				}
				if (line.substr(0, 12) == "    {\"src\":\"") {
					end1 = line.find('\"', 13);
					const std::string key = line.substr(12, end1 - 12);
					// If the segment already exists
					if (json_data.contains(key)) {
					}
					else {
						json_data[key] = line.substr(end1 + 11, line.find('\"', end1 + 12) - end1 - 11);
					}
				}
			} else {
				std::cout << "Malformed Alias JSON\n";
			}
		}
	}

	bool JSON::exists(const std::string& file) const {
		std::string file_lower_case = file;
		std::transform(file_lower_case.begin(), file_lower_case.end(), file_lower_case.begin(), ::tolower);

		if (json_data.contains(file_lower_case)) {
			return true;
		}
		return false;
	}

	std::string JSON::alias(const std::string& file) const {
		std::string file_lower_case = file;
		std::transform(file_lower_case.begin(), file_lower_case.end(), file_lower_case.begin(), ::tolower);
		return json_data.at(file_lower_case);
	}
}