#include "stdafx.h"

namespace json {
	JSON::JSON(const fs::path& path) {
		load(path);
	}

	void JSON::load(const fs::path& path) {
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
						const std::string key = line.substr(12, end1 - 12);
						// If the segment already exists
						if (json_data.count(key)) {
							continue;
						}
						json_data[key] = line.substr(end1 + 11, line.find('\"', end1 + 12) - end1 - 11);
					}
				}
				if (line.substr(0, 12) == "    {\"src\":\"") {
					end1 = line.find('\"', 13);
					const std::string key = line.substr(12, end1 - 12);
					// If the segment already exists
					if (json_data.count(key)) {
					}
					else {
						json_data[key] = line.substr(end1 + 11, line.find('\"', end1 + 12) - end1 - 11);
					}
				}
			} else {
			#ifdef _DEBUG
				std::cout << "Malformed JSON\n";
			#endif
			}
		}
	}

	bool JSON::exists(std::string file) const {
	#ifdef _DEBUG
		std::cout << "Queried existance of alias for: " << file;
	#endif
		if (json_data.count(file))
		{
			#ifdef _DEBUG
			std::cout << "  FOUND\n";
			#endif
				return true;
		}
		#ifdef _DEBUG
		std::cout << "   NOT FOUND\n";
		#endif
		return false;
	}

	std::string JSON::alias(std::string file) const {
		return json_data.at(file);
	}
}