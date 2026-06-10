export module SLK;

import std;
import Hierarchy;
import BinaryReader;
import Utilities;
import INI;
import UnorderedMap;
import no_init_allocator;
import "absl/strings/str_split.h";
import "absl/strings/str_join.h";

namespace fs = std::filesystem;

using namespace std::string_literals;

namespace slk {
	export class SLK {
		/// column_header should be lowercase
		[[nodiscard]]
		std::optional<std::string_view> data_single_asset_type(std::string_view column_header, std::string_view row_header) const {
			assert(to_lowercase_copy(column_header) == column_header);

			// Shadow data
			if (const auto found_row = shadow_data.find(row_header); found_row != shadow_data.end()) {
				if (const auto found_column = found_row->second.find(column_header); found_column != found_row->second.end()) {
					return found_column->second;
				}
			}

			// Base data
			if (const auto found_row = base_data.find(row_header); found_row != base_data.end()) {
				if (const auto found_column = found_row->second.find(column_header); found_column != found_row->second.end()) {
					return found_column->second;
				}
			}

			return {};
		}

		std::expected<void, std::string> load_from_buffer(std::vector<unsigned char, default_init_allocator<unsigned char>> buffer) {
			std::string_view view(reinterpret_cast<const char*>(buffer.data()), buffer.size());

			if (!view.starts_with("ID")) {
				return std::unexpected<std::string>("Does not contain \"ID\" as first record");
			}

			const auto skip_line = [&]() {
				const size_t newline = view.find('\n');
				// No more newlines means end of the file, so consume all
				view.remove_prefix(newline == std::string_view::npos ? view.size() : newline + 1);
			};

			const auto parse_integer = [&]() -> size_t {
				size_t value = 0;
				const size_t separator = view.find(';');
				const size_t end = (separator == std::string_view::npos) ? view.size() : separator;
				if (view.size() >= 2 && end >= 1) {
					std::from_chars(view.data() + 1, view.data() + end, value);
				}
				view.remove_prefix((separator == std::string_view::npos) ? view.size() : separator + 1);
				return value;
			};

			// Skip first ID line
			skip_line();

			size_t column = 0;
			size_t row = 0;

			while (view.size()) {
				switch (view.front()) {
					case 'C':
						view.remove_prefix(2);

						if (!view.empty() && view.front() == 'X') {
							column = parse_integer() - 1;

							if (!view.empty() && view.front() == 'Y') {
								row = parse_integer() - 1;
							}
						} else {
							row = parse_integer() - 1;

							if (!view.empty() && view.front() == 'X') {
								column = parse_integer() - 1;
							}
						}

						// A malformed or 0 index underflows to SIZE_MAX
						if (column == std::string_view::npos || row == std::string_view::npos) {
							skip_line();
							break;
						}

						if (row == 0 && column == 0) {
							skip_line();
							break;
						}

						if (view.empty()) {
							break;
						}
						view.remove_prefix(1);

						{
							std::string data;

							if (!view.empty() && view.front() == '\"') {
								data = view.substr(1, view.find_first_of("\r\n") - 1);
							} else {
								data = view.substr(0, view.find_first_of("\r\n"));
							}

							if (!data.empty() && data.back() == '\"') {
								data.pop_back();
							}

							if (data == "-" || data == "_") {
								data = "";
							}

							if (column == 0) {
								// -1 as 0,0 is unitid/doodadid etc.
								row_headers.emplace(data, row - 1);
								index_to_row.emplace(row - 1, std::move(data));
							} else if (row == 0) {
								// If it is a column header, we need to lowercase it as column headers are case-insensitive
								to_lowercase(data);
								// -1 as 0,0 is unitid/doodadid etc.
								column_headers.emplace(data, column - 1);
								index_to_column.emplace(column - 1, std::move(data));
							} else {
								base_data[index_to_row[row - 1]][index_to_column[column - 1]] = std::move(data);
							}

							skip_line();
						}
						break;
					case 'F':
						if (!view.empty() && view.front() == 'X') {
							column = parse_integer() - 1;

							if (!view.empty() && view.front() == 'Y') {
								row = parse_integer() - 1;
							}
						} else {
							row = parse_integer() - 1;

							if (!view.empty() && view.front() == 'X') {
								column = parse_integer() - 1;
							}
						}
						skip_line();
						break;
					default:
						skip_line();
				}
			}

			// Remove empty rows (might contain data but don't have a row header)

			size_t i = 0;
			while (i < index_to_row.size()) {
				if (index_to_row.at(i).empty()) {
					for (size_t j = index_to_row.size() - 1; j > i; j--) {
						if (!index_to_row.at(j).empty()) {
							index_to_row[i] = index_to_row.at(j);
							row_headers.at(index_to_row.at(i)) = i;
							index_to_row.erase(j);
							break;
						}
					}
				}
				i += 1;
			}
			row_headers.erase("");

			return {};
		}

	  public:
		hive::unordered_map<size_t, std::string> index_to_row;
		hive::unordered_map<size_t, std::string> index_to_column;
		hive::unordered_map<std::string, size_t> row_headers;
		hive::unordered_map<std::string, size_t> column_headers;
		hive::unordered_map<std::string, hive::unordered_map<std::string, std::string>> base_data;
		hive::unordered_map<std::string, hive::unordered_map<std::string, std::string>> shadow_data;

		// The following map is only used in meta SLKs and maps the field (+unit/ability ID) to a meta ID
		hive::unordered_map<std::string, std::string> meta_map;

		SLK() = default;

		/// Constructs an SLK and immediately loads it from the hierarchy
		explicit SLK(const fs::path& path, const Hierarchy::FileSource source = Hierarchy::FileSource::all) {
			// Rethrow the error string itself so callers (e.g. Map::load's catch) report the real
			// cause rather than the generic std::bad_expected_access message that .value() would throw.
			if (auto result = load_hierarchy(path, source); !result) {
				throw std::runtime_error(result.error());
			}
		}

		/// Loads an SLK file from the hierarchy using the specified source flags (overrides, imports, local files, casc)
		std::expected<void, std::string>
		load_hierarchy(const fs::path& path, const Hierarchy::FileSource source = Hierarchy::FileSource::all) {
			auto res = hierarchy.open_file(path, source);
			if (!res) {
				return std::unexpected(res.error());
			}

			return load_from_buffer(std::move(res->buffer));
		}

		/// Loads an SLK file from the disk. The path is relative to the editor executable
		std::expected<void, std::string> load_local(const fs::path& path) {
			auto res = read_file(path);
			if (!res) {
				return std::unexpected(res.error());
			}

			return load_from_buffer(std::move(res->buffer));
		}

		void build_meta_map() {
			// Check if we are a meta_slk
			if (!column_headers.contains("field")) {
				return;
			}

			for (const auto& [header, row] : row_headers) {
				std::string field = to_lowercase_copy(data<std::string_view>("field", header));

				const int repeat = data<int>("data", header);
				if (repeat > 0) {
					field += 'a' + (repeat - 1);
				}
				if (column_headers.contains("usespecific")) {
					std::vector<std::string> parts = absl::StrSplit(data<std::string_view>("usespecific", header), ",", absl::SkipEmpty());
					if (!parts.empty()) {
						for (const auto& i : parts) {
							meta_map.emplace(field + i, header);
						}
					} else {
						meta_map.emplace(field, header);
					}
				} else {
					meta_map.emplace(field, header);
				}
			}
		}

		/// To map a field in a data SLK (race, pathTex, moveSpeed, etc.) to the field ID in the meta SLK.
		/// The ID of the unit/doodad/ability needs to be supplied as some fields can only be resolved that way (useSpecific for abilities).
		[[nodiscard]]
		std::optional<std::string_view>
		field_to_meta_id(const SLK& meta_slk, const std::string_view field_name, const std::string_view id) const {
			// First check raw field name. They can sometimes have numbers already (effect1, mod2, etc.)
			if (const auto found_field = meta_slk.meta_map.find(field_name); found_field != meta_slk.meta_map.end()) {
				return found_field->second;
			}

			// Then strip the number as it then probably is just a variation number (e.g. name1 and name2 for different upgrade levels)
			const std::string_view stripped_field_name = field_name.substr(0, field_name.find_first_of("0123456789"));
			if (const auto found_field = meta_slk.meta_map.find(stripped_field_name); found_field != meta_slk.meta_map.end()) {
				return found_field->second;
			}

			// Only abilities should get here as they're the only ones that have multiple field names mapping to the same field ID
			std::string_view base_id = id;
			if (const auto found_base_id = data_single_asset_type("oldid", id); found_base_id) {
				base_id = found_base_id.value();
			}

			// Abilities can also alias another existing ability, so we have to check both the base ID and alias
			// Sometimes only the base ID is used in `useSpecific` and sometimes only the alias.

			// Safety: Only abilities should enter this block and they all have an alias
			const auto alias = data_single_asset_type("code", id).value();
			const auto found_alias = meta_slk.meta_map.find(std::string(stripped_field_name).append(alias));
			if (found_alias != meta_slk.meta_map.end()) {
				return found_alias->second;
			}

			const auto found = meta_slk.meta_map.find(std::string(stripped_field_name).append(base_id));
			if (found != meta_slk.meta_map.end()) {
				return found->second;
			}

			return {};
		}

		// column_header should be lowercase
		template<typename T = std::string>
		T data(const std::string_view column_header, const std::string_view row_header) const {
			static_assert(
				std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string> || std::is_floating_point_v<T>
					|| std::is_integral_v<T>,
				"Type not supported. Convert yourself or add conversion here if it makes sense"
			);
			assert(to_lowercase_copy(column_header) == column_header);

			auto data = data_single_asset_type(column_header, row_header);
			if (!data) {
				if (hierarchy.hd) {
					data = data_single_asset_type(std::string(column_header) + ":hd", row_header);
				} else {
					data = data_single_asset_type(std::string(column_header) + ":sd", row_header);
				}
			}

			if (!data) {
				return T();
			}

			if constexpr (std::is_same<T, std::string_view>()) {
				return *data;
			} else if constexpr (std::is_same<T, std::string>()) {
				return std::string(*data);
			} else if constexpr (std::is_same<T, bool>()) {
				int output;
				std::from_chars(data->data(), data->data() + data->size(), output);
				return output != 0;
			} else if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>) {
				T output;
				std::from_chars(data->data(), data->data() + data->size(), output);
				return output;
			}

			throw;
		}

		// Gets the data by first checking the shadow table and then checking the base table
		// Does :sd and :hd tag resolution too
		// column_header should be lowercase
		// If you have both an integer row index and the string row name then use the overload that takes string_view as it will do a index->name conversion internally
		template<typename T = std::string>
		T data(const std::string_view column_header, const size_t row) const {
			if (row >= index_to_row.size()) {
				throw;
			}

			return data<T>(column_header, index_to_row.at(row));
		}

		// Gets the data by first checking the shadow table and then checking the base table
		template<typename T = std::string>
		T data(const size_t column, const size_t row) const {
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
		void merge(const SLK& slk) {
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

		/// Merges the data of the files. INI sections are matched to row keys, and INI keys are matched to column keys.
		/// If an unknown section key is encountered, then that section is skipped.
		/// If an unknown column key is encountered, then the column is added

		size_t non_matches = 0;

		void merge(const ini::INI& ini, const SLK& meta_slk) {
			for (const auto& [section_key, section_value] : ini.ini_data) {
				auto found_section = base_data.find(section_key);
				if (found_section == base_data.end()) {
					continue;
				}

				auto& [key, section] = *found_section;

				for (const auto& [key, value] : section_value) {
					const std::string key_lower = to_lowercase_copy(key);

					if (!column_headers.contains(key_lower)) {
						add_column(key_lower);
					}

					// By making some changes to unitmetadata.slk and unitdata.slk we can avoid the 1->2->2 mapping for SLK->OE->W3U files.
					// This means we have to manually split these into the correct column
					if (value.size() > 1
						&& (key_lower == "missilearc" || key_lower == "missileart" || key_lower == "missilehoming"
							|| key_lower == "missilespeed" || key_lower == "buttonpos" || key_lower == "unbuttonpos"
							|| key_lower == "researchbuttonpos")
						&& column_headers.contains(key_lower + "2")) {
						section[key_lower] = value[0];
						section[key_lower + "2"] = value[1];
						continue;
					}

					const std::string key_lower_stripped = key_lower.substr(0, key_lower.find_first_of(':'));

					std::string id;
					if (auto found = meta_slk.meta_map.find(key_lower_stripped); found != meta_slk.meta_map.end()) {
						id = found->second;
					} else if (auto found = meta_slk.meta_map.find(key_lower_stripped + section_key); found != meta_slk.meta_map.end()) {
						id = found->second;
					} else {
						const size_t nr_position = key_lower_stripped.find_first_of("0123456789");
						const std::string without_numbers = key_lower_stripped.substr(0, nr_position);

						if (auto found = meta_slk.meta_map.find(without_numbers); found != meta_slk.meta_map.end()) {
							id = found->second;
						} else {
							continue;
						}
					}

					const int repeat = meta_slk.data<int>("repeat", id);
					if (repeat > 0 && !(meta_slk.column_headers.contains("appendindex") && meta_slk.data<int>("appendindex", id) > 0)) {
						for (size_t i = 0; i < value.size(); i++) {
							const std::string new_key = std::format("{}{}", key_lower, i + 1);
							if (!column_headers.contains(new_key)) {
								add_column(new_key);
							}
							section[new_key] = value[i];
						}
					} else {
						if (meta_slk.data<std::string_view>("type", id).ends_with("List")) {
							section[key_lower] = absl::StrJoin(value, ",");
						} else {
							section[key_lower] = value[0];
						}
					}
				}
			}
		}

		/// Substitutes the data of the slk with data from the INI based on a certain section key.
		/// The keys of the section are matched with all the cells in the table and if they match will replace the value
		void substitute(const ini::INI& ini, const std::string_view section) {
			assert(ini.section_exists(section));

			for (auto& [id, properties] : base_data) {
				for (auto& [prop_id, prop_value] : properties) {
					const std::string_view data = ini.data<std::string_view>(section, prop_value);
					if (!data.empty()) {
						prop_value = std::string(data);
					}
				}
			}
		}

		void add_row(const std::string_view row_header) {
			assert(!base_data.contains(row_header));

			size_t index = row_headers.size();
			row_headers.emplace(row_header, index);
			index_to_row[index] = row_header;
		}

		/// Copies the row with header row_header to a new line with the new header as new_row_header
		void copy_row(const std::string_view row_header, std::string_view new_row_header, bool copy_shadow_data) {
			assert(base_data.contains(row_header));
			assert(!base_data.contains(new_row_header));

			base_data[new_row_header] = base_data.at(row_header);

			if (copy_shadow_data && shadow_data.contains(row_header)) {
				shadow_data[new_row_header] = shadow_data.at(row_header);
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
				// Swap with an element from the end to avoid having to change all indices
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

			// If the shadow data is equal to to the base data we remove the shadow data
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

			shadow_data[row_header][column_header] = std::move(data);
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
