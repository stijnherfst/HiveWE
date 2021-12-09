#include "INI.h"

#include "Hierarchy.h"
#include "Utilities.h"

#include <absl/strings/str_split.h>
#include <fstream>

namespace ini {
	INI::INI(const fs::path& path, bool local) {
		load(path, local);
	}

	void INI::load(const fs::path& path, bool local) {
		std::vector<uint8_t, default_init_allocator<uint8_t>> buffer;
		if (local) {
			std::ifstream stream(path, std::ios::binary);
			buffer = std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		} else {
			buffer = hierarchy.open_file(path).buffer;
		}
		std::string_view view(reinterpret_cast<char*>(buffer.data()), buffer.size());

		// Strip byte order marking
		if (view.starts_with(std::string{ (char)0xEF, (char)0xBB, (char)0xBF })) {
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

				ini_data[std::string(current_section)][std::string(key)] = parts;
			}
			view.remove_prefix(eol + 1);
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
		if (ini_data.contains(section)) {
			return ini_data.at(section);
		} else {
			return {};
		}
	}

	void INI::set_whole_data(const std::string& section, const std::string& key, std::string value) {
		ini_data[section][key] = { value };
	}

	std::vector<std::string> INI::whole_data(const std::string& section, const std::string& key) const {
		if (ini_data.contains(section) && ini_data.at(section).contains(key)) { // ToDo C++20 contains
			return ini_data.at(section).at(key);
		} else {
			return {};
		}
	}

	bool INI::key_exists(const std::string& section, const std::string& key) const {
		return ini_data.contains(section) && ini_data.at(section).contains(key);
	}

	bool INI::section_exists(const std::string& section) const {
		return ini_data.contains(section);
	}
}