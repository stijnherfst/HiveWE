module;

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <absl/strings/str_split.h>

export module INI;

import Utilities;
import Hierarchy;
import no_init_allocator;

namespace fs = std::filesystem;

namespace ini {
	export class INI {
	  public:
		/// header to items to list of values to value
		// ToDo use ankerl::unordered_dense
		std::map<std::string, std::map<std::string, std::vector<std::string>>> ini_data;

		INI() = default;
		explicit INI(const fs::path& path, bool local = false) {
			load(path, local);
		}

		void load(const fs::path& path, bool local = false) {
			std::vector<uint8_t, default_init_allocator<uint8_t>> buffer;
			if (local) {
				std::ifstream stream(path, std::ios::binary);
				buffer = std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
			} else {
				buffer = hierarchy.open_file(path).buffer;
			}
			std::string_view view(reinterpret_cast<char*>(buffer.data()), buffer.size());

			// Strip byte order marking
			if (view.starts_with(std::string{ static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) })) {
				view.remove_prefix(3);
			}

			std::string_view current_section;

			while (view.size()) {
				size_t eol = view.find('\n');
				if (eol == std::string_view::npos) {
					eol = view.size() - 1;
				}

				if (view.starts_with("//") || view.starts_with(';') || view.starts_with('\n') || view.starts_with('\r')) {
					view.remove_prefix(eol + 1);
					continue;
				}

				if (view.front() == '[') {
					current_section = view.substr(1, view.find(']') - 1);
				} else {
					const size_t found = view.find_first_of('=');
					if (found == std::string_view::npos) {
						view.remove_prefix(eol + 1);
						continue;
					}

					const std::string_view key = view.substr(0, found);
					const std::string_view value = view.substr(found + 1, view.find_first_of("\r\n") - 1 - found);

					if (key.empty() || value.empty()) {
						view.remove_prefix(eol + 1);
						continue;
					}

					std::vector<std::string> parts;

					if (value.front() == '\"') {
						parts = absl::StrSplit(value, "\",\"");
					} else {
						parts = absl::StrSplit(value, ',');
					}

					// Strip off quotes at the front/back
					for (auto&& i : parts) {
						if (i.size() < 2) {
							continue;
						}
						if (i.front() == '\"') {
							i.erase(i.begin());
						}
						if (i.back() == '\"') {
							i.pop_back();
						}
					}

					// Sometimes there are duplicate keys and only the first seen value has to be retained
					// E.g. the destructable LTt0 in destructableskin.txt has multiple minScale/maxScale
					auto current_section_string = std::string(current_section);
					auto key_string = std::string(key);
					if (auto found = ini_data.find(current_section_string); found != ini_data.end()) {
						if (!found->second.contains(key_string)) {
							found->second[key_string] = parts;	
						}
					} else {
						ini_data[current_section_string][key_string] = parts;	
					}
				}
				view.remove_prefix(eol + 1);
			}
		}

		/// Replaces all values (not keys) which match one of the keys in substitution INI
		void substitute(const INI& ini, const std::string& section) {
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

		std::map<std::string, std::vector<std::string>> section(const std::string& section) const {
			if (ini_data.contains(section)) {
				return ini_data.at(section);
			} else {
				return {};
			}
		}

		/// Sets the data of a whole key
		void set_whole_data(const std::string& section, const std::string& key, std::string value) {
			ini_data[section][key] = { value };
		}

		/// Retrieves the list of key values
		std::vector<std::string> whole_data(const std::string& section, const std::string& key) const {
			if (ini_data.contains(section) && ini_data.at(section).contains(key)) { // ToDo C++20 contains
				return ini_data.at(section).at(key);
			} else {
				return {};
			}
		}

		bool key_exists(const std::string& section, const std::string& key) const {
			return ini_data.contains(section) && ini_data.at(section).contains(key);
		}

		bool section_exists(const std::string& section) const {
			return ini_data.contains(section);
		}

		/// To access key data where the value of the key is comma seperated
		template <typename T = std::string>
		T data(const std::string& section, const std::string& key, const size_t argument = 0) const {
			if (ini_data.contains(section) && ini_data.at(section).contains(key) && argument < ini_data.at(section).at(key).size()) {
				if constexpr (std::is_same<T, std::string>()) {
					return ini_data.at(section).at(key)[argument];
				} else if constexpr (std::is_same<T, int>()) {
					return std::stoi(ini_data.at(section).at(key)[argument]);
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(ini_data.at(section).at(key)[argument]);
				}
				static_assert("Type not supported. Convert yourself or add conversion here if it makes sense");
			} else {
				return T();
			}
		}
	};
} // namespace ini