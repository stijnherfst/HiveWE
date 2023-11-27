module;

#include <string>
#include <unordered_map>
#include <filesystem>
#include <cassert>
#include <fstream>
#include <numeric>
#include <string_view>
#include <charconv>
#include <print>

#include <absl/strings/str_split.h>
#include <absl/strings/str_join.h>

#include "ankerl/unordered_dense.h"

export module SLK;

namespace fs = std::filesystem;

import Hierarchy;
import no_init_allocator;
import BinaryReader;
import Utilities;
import INI;

#undef mix
#undef max

using namespace std::string_literals;

namespace slk {
	// To enable heterogeneous lookup
	struct string_hash {
		using is_transparent = void; // enable heterogeneous lookup
		using is_avalanching = void; // mark class as high quality avalanching hash

		[[nodiscard]] auto operator()(const char* str) const noexcept -> uint64_t {
			return ankerl::unordered_dense::hash<std::string_view>{}(str);
		}

		[[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
			return ankerl::unordered_dense::hash<std::string_view>{}(str);
		}

		[[nodiscard]] auto operator()(std::string const& str) const noexcept -> uint64_t {
			return ankerl::unordered_dense::hash<std::string_view>{}(str);
		}
	};

	export class SLK {

	  public:
		ankerl::unordered_dense::map<size_t, std::string> index_to_row;
		ankerl::unordered_dense::map<size_t, std::string> index_to_column;
		ankerl::unordered_dense::map<std::string, size_t, string_hash, std::equal_to<>> row_headers;
		ankerl::unordered_dense::map<std::string, size_t, string_hash, std::equal_to<>> column_headers;
		ankerl::unordered_dense::map<std::string, ankerl::unordered_dense::map<std::string, std::string, string_hash, std::equal_to<>>, string_hash, std::equal_to<>> base_data;
		ankerl::unordered_dense::map<std::string, ankerl::unordered_dense::map<std::string, std::string, string_hash, std::equal_to<>>, string_hash, std::equal_to<>> shadow_data;

		// The following map is only used in meta SLKs and maps the field (+unit/ability ID) to a meta ID
		ankerl::unordered_dense::map<std::string, std::string, string_hash, std::equal_to<>> meta_map;

		SLK() = default;
		explicit SLK(const fs::path& path, const bool local = false) {
			load(path, local);
		}

		void load(const fs::path& path, const bool local = false) {
			std::vector<uint8_t, default_init_allocator<uint8_t>> buffer;
			if (local) {
				std::ifstream stream(path, std::ios::binary);
				buffer = std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
			} else {
				buffer = hierarchy.open_file(path).buffer;
			}

			std::string_view view(reinterpret_cast<char*>(buffer.data()), buffer.size());

			if (!view.starts_with("ID")) {
				std::print("Invalid SLK file, does not contain \"ID\" as first record\n");
				return;
			}

			const auto parse_integer = [&]() {
				size_t value;
				size_t separator = view.find(';');
				std::from_chars(&view[1], &view[separator], value);
				view.remove_prefix(separator + 1);
				return value;
			};

			// Skip first ID line
			view.remove_prefix(view.find('\n') + 1);

			size_t column = 0;
			size_t row = 0;

			while (view.size()) {
				switch (view.front()) {
					case 'C':
						view.remove_prefix(2);

						if (view.front() == 'X') {
							column = parse_integer() - 1;

							if (view.front() == 'Y') {
								row = parse_integer() - 1;
							}
						} else {
							row = parse_integer() - 1;

							if (view.front() == 'X') {
								column = parse_integer() - 1;
							}
						}

						if (row == 0 && column == 0) {
							view.remove_prefix(view.find('\n') + 1);
							break;
						}

						view.remove_prefix(1);

						{
							std::string data;
							if (view.front() == '\"') {
								data = view.substr(1, view.find('"', 1) - 1);
							} else {
								data = view.substr(0, view.find_first_of("\r\n"));
							}

							if (data == "-" || data == "_") {
								data = "";
							}

							if (column == 0) {
								// -1 as 0,0 is unitid/doodadid etc.
								row_headers.emplace(data, row - 1);
								index_to_row.emplace(row - 1, data);
							} else if (row == 0) {
								// If it is a column header we need to lowercase it as column headers are case insensitive
								to_lowercase(data);
								// -1 as 0,0 is unitid/doodadid etc.
								column_headers.emplace(data, column - 1);
								index_to_column.emplace(column - 1, data);
							} else {
								base_data[index_to_row[row - 1]][index_to_column[column - 1]] = data;
							}

							view.remove_prefix(view.find('\n') + 1);
						}
						break;
					case 'F':
						if (view.front() == 'X') {
							column = parse_integer() - 1;

							if (view.front() == 'Y') {
								row = parse_integer() - 1;
							}
						} else {
							row = parse_integer() - 1;

							if (view.front() == 'X') {
								column = parse_integer() - 1;
							}
						}
						view.remove_prefix(view.find('\n') + 1);
						break;
					default:
						view.remove_prefix(view.find_first_of('\n') + 1);
				}
			}
		}

		void build_meta_map() {
			// Check if we are a meta_slk
			if (!column_headers.contains("field")) {
				return;
			}

			for (const auto& [header, row] : row_headers) {
				std::string field = to_lowercase_copy(data("field", header));

				const int repeat = data<int>("data", header);
				if (repeat > 0) {
					field += 'a' + (repeat - 1);
				}
				if (column_headers.contains("usespecific")) {
					std::vector<std::string> parts = absl::StrSplit(data("usespecific", header), ",");
					if (!parts.empty()) {
						for (const auto& i : parts) {
							meta_map.emplace(field, header + i);
						}
					} else {
						meta_map.emplace(field, header);
					}
				} else {
					meta_map.emplace(field, header);
				}
			}
		}

		// column_header should be lowercase
		template <typename T = std::string>
		T data(std::string_view column_header, std::string_view row_header) const {
			static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, bool>,  "Type not supported. Convert yourself or add conversion here if it makes sense");
			assert(to_lowercase_copy(column_header) == column_header);

			// Shadow data
			if (const auto found_row = shadow_data.find(row_header); found_row != shadow_data.end()) {
				if (const auto found_column = found_row->second.find(column_header); found_column != found_row->second.end()) {
					if constexpr (std::is_same<T, std::string>()) {
						return found_column->second;
					} else if constexpr (std::is_same<T, float>()) {
						return std::stof(found_column->second);
					} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
						return std::stoi(found_column->second);
					}
				}
			}

			if (hierarchy.hd) {
				const auto column_header_hd = std::string(column_header) + ":hd";
				if (const auto found = column_headers.find(column_header_hd); found != column_headers.end()) {
					const std::string hd_data = data(column_header_hd, row_header);
					if (!hd_data.empty()) { // ToDo What if I clear the model field in HD mode. Will it try loading the SD model then because we don't return the blank line?
						if constexpr (std::is_same<T, std::string>()) {
							return hd_data;
						} else if constexpr (std::is_same<T, float>()) {
							return std::stof(hd_data);
						} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
							return std::stoi(hd_data);
						}
					}
				}
			}

			// Base data
			if (const auto found_row = base_data.find(row_header); found_row != base_data.end()) {
				if (const auto found_column = found_row->second.find(column_header); found_column != found_row->second.end()) {
					if constexpr (std::is_same<T, std::string>()) {
						return found_column->second;
					} else if constexpr (std::is_same<T, float>()) {
						return std::stof(found_column->second);
					} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
						return std::stoi(found_column->second);
					}
				}
			}

			return T();
		}

		// Gets the data by first checking the shadow table and then checking the base table
		// Also does :hd tag resolution
		// column_header should be lowercase
		// If you have both an integer row index and the string row name then use the overload that takes string_view as it will do a index->name conversion internally
		template <typename T = std::string>
		T data(const std::string_view column_header, size_t row) const {
			if (row >= index_to_row.size()) {
				throw;
			}

			return data<T>(column_header, index_to_row.at(row));
		}

		// Gets the data by first checking the shadow table and then checking the base table
		template <typename T = std::string>
		T data(size_t column, size_t row) const {
			if (row >= index_to_row.size()) {
				throw;
			}

			if (column >= index_to_column.size()) {
				throw;
			}

			return data<T>(index_to_column.at(column), index_to_row.at(row));
		}

		// Merges the base data of the files
		// Shadow data is not merged
		// Any unknown columns are appended
		void merge(const slk::SLK& slk) {
			for (const auto& [header, index] : slk.column_headers) {
				if (!column_headers.contains(header)) {
					add_column(header);
				}
			}

			for (const auto& [id, properties] : slk.base_data) {
				if (!base_data.contains(id)) {
					continue;
				}
				base_data[id].insert(properties.begin(), properties.end());
			}
		}

		/// Merges the data of the files. INI sections are matched to row keys and INI keys are matched to column keys.
		/// If an unknown section key is encountered then that section is skipped
		/// If an unknown column key is encountered then the column is added
		void merge(const ini::INI& ini, const SLK& meta_slk) {
			for (const auto& [section_key, section_value] : ini.ini_data) {
				if (!base_data.contains(section_key)) {
					continue;
				}

				for (const auto& [key, value] : section_value) {
					std::string key_lower = to_lowercase_copy(key);

					if (!column_headers.contains(key_lower)) {
						add_column(key_lower);
					}

					// By making some changes to unitmetadata.slk and unitdata.slk we can avoid the 1->2->2 mapping for SLK->OE->W3U files.
					// This means we have to manually split these into the correct column
					if (value.size() > 1 && (key_lower == "missilearc" || key_lower == "missileart" || key_lower == "missilehoming" || key_lower == "missilespeed" || key_lower == "buttonpos" || key_lower == "unbuttonpos" || key_lower == "researchbuttonpos") && column_headers.contains(key_lower + "2")) {

						base_data[section_key][key_lower] = value[0];
						base_data[section_key][key_lower + "2"] = value[1];
						continue;
					}

					const std::string key_lower_stripped = key_lower.substr(0, key_lower.find_first_of(':'));
					
					std::string id;
					if (auto found = meta_slk.meta_map.find(key_lower_stripped); found != meta_slk.meta_map.end()) {
						id = found->second;
					} else if (auto found = meta_slk.meta_map.find(key_lower_stripped + section_key); found != meta_slk.meta_map.end()) {
						id = found->second;
					} else {
						size_t nr_position = key_lower_stripped.find_first_of("0123456789");
						std::string without_numbers = key_lower_stripped.substr(0, nr_position);

						if (auto found = meta_slk.meta_map.find(without_numbers); found != meta_slk.meta_map.end()) {
							id = found->second;
						} else {
							continue;
						}
					}

					//std::string id;
					//if (meta_slk.meta_map.contains(key_lower_stripped)) {
					//	id = meta_slk.meta_map.at(key_lower_stripped);
					//} else if (meta_slk.meta_map.contains(key_lower_stripped + section_key)) {
					//	id = meta_slk.meta_map.at(key_lower_stripped + section_key);
					//} else {
					//	size_t nr_position = key_lower_stripped.find_first_of("0123456789");
					//	std::string without_numbers = key_lower_stripped.substr(0, nr_position);

					//	if (meta_slk.meta_map.contains(without_numbers)) {
					//		id = meta_slk.meta_map.at(without_numbers);
					//	} else {
					//		continue;
					//	}
					//}

					const int repeat = meta_slk.data<int>("repeat", id);
					if (repeat > 0 && !(meta_slk.column_headers.contains("appendindex") && meta_slk.data<int>("appendindex", id) > 0)) {
						for (size_t i = 0; i < value.size(); i++) {
							const std::string new_key = key_lower + std::to_string(i + 1);
							if (!column_headers.contains(new_key)) {
								add_column(new_key);
							}
							base_data[section_key][new_key] = value[i];
						}
						continue;
					} else {
						if (meta_slk.data<std::string>("type", id).ends_with("List")) {
							base_data[section_key][key_lower] = absl::StrJoin(value, ",");
						} else {
							base_data[section_key][key_lower] = value[0];
						}
					}
				}
			}
		}

		/// Substitutes the data of the slk with data from the INI based on a certain section key.
		/// The keys of the section are matched with all the cells in the table and if they match will replace the value
		void substitute(const ini::INI& ini, const std::string& section) {
			assert(ini.section_exists(section));

			for (auto& [id, properties] : base_data) {
				for (auto& [prop_id, prop_value] : properties) {
					std::string data = ini.data(section, prop_value);
					if (!data.empty()) {
						prop_value = data;
					}
				}
			}
		}

		/// Copies the row with header row_header to a new line with the new header as new_row_header
		void copy_row(const std::string_view row_header, std::string_view new_row_header, bool copy_shadow_data) {
			assert(base_data.contains(row_header));
			assert(!base_data.contains(new_row_header));

			// Get a weird allocation error if not done via a temporary 19/06/2021
			const auto t = base_data.at(row_header);
			base_data[new_row_header] = t;

			if (copy_shadow_data && shadow_data.contains(row_header)) {
				// Get a weird allocation error if not done via a temporary 19/06/2021
				const auto tt = shadow_data.at(row_header);
				shadow_data[new_row_header] = tt;
			}

			size_t index = row_headers.size();
			row_headers.emplace(new_row_header, index);
			index_to_row[index] = new_row_header;

			// Only set/change oldid if the row didn't have one (which means it is a default unit/item/...)
			if (!shadow_data[new_row_header].contains("oldid")) {
				shadow_data[new_row_header]["oldid"] = row_header;
			}
		}

		void remove_row(const std::string_view row_header) {
			assert(base_data.contains(row_header));

			base_data.erase(row_header);
			shadow_data.erase(row_header);

			const size_t index = row_headers.at(row_header);
			if (index == rows() - 1) {
				index_to_row.erase(index);
				row_headers.erase(row_header);
			} else {
				// Swap with a element from the end to avoid having to change all indices
				const std::string replacement_id = index_to_row.at(rows() - 1);
				index_to_row[index] = replacement_id;
				row_headers[replacement_id] = index;
				index_to_row.erase(rows() - 1);

				row_headers.erase(row_header);
			}
		}

		/// Adds a (virtual) column
		/// Since SLK2 is only a key/pair store it emulates being table like and thus this call is very cheap memory/cpu wise
		/// column_header must be lowercase
		void add_column(const std::string_view column_header) {
			assert(to_lowercase_copy(column_header) == column_header);

			size_t index = column_headers.size();
			column_headers.emplace(column_header, index);
			index_to_column[index] = column_header;
		}

		// column_header should be lowercase
		void set_shadow_data(const std::string_view column_header, const std::string_view row_header, std::string data) {
			assert(to_lowercase_copy(column_header) == column_header);

			if (!column_headers.contains(column_header)) {
				add_column(column_header);
			}

			if (base_data.contains(row_header) && base_data.at(row_header).contains(column_header)) {
				if (base_data.at(row_header).at(column_header) == data) {
					if (shadow_data.contains(row_header)) {
						shadow_data.at(row_header).erase(column_header);
						if (shadow_data.at(row_header).empty()) {
							shadow_data.erase(row_header);
						}
					}
					return;
				}
			}

			shadow_data[row_header][column_header] = data;
		}

		void set_shadow_data(const int column, const int row, std::string data) {
			set_shadow_data(index_to_column.at(column), index_to_row.at(row), data);
		}

		size_t rows() const {
			return row_headers.size();
		}

		size_t columns() const {
			return column_headers.size();
		}
	};
} // namespace slk
