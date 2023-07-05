module;

#include <map>
#include <string>
#include <sstream>
#include <iostream>

export module TriggerStrings;

import BinaryReader;
import BinaryWriter;
import Hierarchy;

export class TriggerStrings {
	std::map<std::string, std::string> strings; // ToDo change back to unordered_map?

	size_t next_id;

  public:
	void load() {
		BinaryReader reader = hierarchy.map_file_read("war3map.wts");

		std::stringstream file;
		file.write(reinterpret_cast<char*>(reader.buffer.data()), reader.buffer.size());

		std::string key;
		std::string line;
		while (std::getline(file, line)) {
			if (line.empty() || line.substr(0, 2) == "//") {
				continue;
			}
			if (line.back() == '\r') {
				line.pop_back();
			}

			if (line.empty()) {
				continue;
			}

			line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return c == '\r'; }), line.end());

			if (line.front() == '{') {
				std::string value;
				bool first = true;
				while (std::getline(file, line) && !line.empty() && line.front() != '}') {
					if (line.back() == '\r') {
						line.pop_back();
					}
					value += (first ? "" : "\n") + line;
					first = false;
				}
				strings.emplace(key, value);
			} else {
				size_t found = line.find(' ') + 1;
				next_id = std::max(next_id, found);
				int padsize = std::max(0, 3 - ((int)line.size() - (int)found));
				key = "TRIGSTR_" + std::string(padsize, '0') + line.substr(found);
			}
		}
	}

	void save() const {
		BinaryWriter writer;

		writer.write<uint8_t>(0xEF);
		writer.write<uint8_t>(0xBB);
		writer.write<uint8_t>(0xBF);

		std::stringstream file;
		for (auto&& [key, value] : strings) {
			auto found = key.find('_') + 1;
			std::string final_key = "STRING " + key.substr(found);

			// Remove leading zeroes
			while (final_key.front() == '0') {
				final_key.erase(final_key.begin());
			}

			std::string final_value = value;
			// Insert carriage returns
			auto it = final_value.begin();
			while (it != final_value.end()) {
				if (*it == '\n') {
					it = final_value.insert(it, '\r');
					it++;
				}
				it++;
			}

			writer.write_string(final_key);
			writer.write_string("\r\n{\r\n");
			writer.write_string(final_value);
			writer.write_string("\r\n}\r\n\r\n");
		}

		hierarchy.map_file_write("war3map.wts", writer.buffer);
	}

	std::string string(const std::string& key) const {
		if (!strings.contains(key)) {
			return "";
		}

		return strings.at(key);
	}

	/// If the key exists then the correspending string in the trigger string file is set
	/// If the key does not exist AND the key empty AND the value is not empty then a string reference is created and assigned to the key variable
	void set_string(std::string& key, const std::string& value) {
		if (key.rfind("TRIGSTR_", 0) != 0) {
			if (key.empty() && !value.empty()) {
				const int padsize = std::max(0, 2 - static_cast<int>(std::log10(next_id)));
				key = "TRIGSTR_" + std::string(padsize, '0') + std::to_string(++next_id);
				strings[key] = value;
				std::cout << "Creating key: " << key << "  " << value << "\n";

				return;
			}
			std::cout << "Invalid TRIGSTR set: " << key << " " << value << "\n";
			return;
		}

		strings[key] = value;
	}
};