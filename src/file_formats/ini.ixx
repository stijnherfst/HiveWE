export module INI;

import std;
import Utilities;
import Hierarchy;
import no_init_allocator;
import <absl/strings/str_split.h>;
import UnorderedMap;

namespace fs = std::filesystem;

namespace ini {
	export class INI {
	  public:
		/// header to items to list of values to value
		hive::unordered_map<std::string, hive::unordered_map<std::string, std::vector<std::string>>> ini_data;

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

			while (!view.empty()) {
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
		void substitute(const INI& ini, const std::string_view section) {
			for (auto&& [section_key, section_value] : ini_data) {
				for (auto&& [key, value] : section_value) {
					for (auto&& part : value) {
						std::string_view we_string = ini.data<std::string_view>(section, part);
						if (!we_string.empty()) {
							part = we_string;
						}
					}
				}
			}
		}

		[[nodiscard]] const hive::unordered_map<std::string, std::vector<std::string>>& section(const std::string_view section) const {
			if (auto found = ini_data.find(section); found != ini_data.end()) {
				return found->second;
			} else {
				throw std::runtime_error("section not found");
			}
		}

		/// Sets the data of a whole key
		void set_whole_data(const std::string_view section, const std::string_view key, std::string value) {
			ini_data[section][key] = { std::move(value) };
		}

		[[nodiscard]] bool key_exists(const std::string_view section, const std::string_view key) const {
			return ini_data.contains(section) && ini_data.at(section).contains(key);
		}

		[[nodiscard]] bool section_exists(const std::string_view section) const {
			return ini_data.contains(section);
		}

		/// To access key data where the value of the key is comma seperated
		template <typename T = std::string>
		[[nodiscard]] T data(const std::string_view section, const std::string_view key, const size_t argument = 0) const {
			const auto sec = ini_data.find(section);
			if (sec == ini_data.end()) {
				throw std::runtime_error("section not found");
			}
			const auto value = sec->second.find(key);
			if (value == sec->second.end()) {
				// Returning an empty value is kind of cursed
				return T{};
				// throw std::runtime_error("key not found");
			}

			if (argument >= value->second.size()) {
				// Returning an empty value is kind of cursed
				return T{};
				// throw std::runtime_error("section argument out of bounds");
			}

			if constexpr (std::is_same_v<T, std::string_view>) {
				return value->second[argument];
			} else if constexpr (std::is_same_v<T, std::string>) {
				return value->second[argument];
			} else if constexpr (std::is_same_v<T, int>) {
				return std::stoi(value->second[argument]);
			} else if constexpr (std::is_same_v<T, float>) {
				return std::stof(value->second[argument]);
			} else  {
				static_assert(false, "Type not supported. Convert yourself or add conversion here if it makes sense");
			}
		}
	};
} // namespace ini