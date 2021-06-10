#pragma once

#include "INI.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include "Hierarchy.h"

#include <absl/container/flat_hash_map.h>

extern std::string to_lowercase_copy(const std::string_view& string);
namespace slk {
	class SLK {

	public:
		absl::flat_hash_map<size_t, std::string> index_to_row;
		absl::flat_hash_map<size_t, std::string> index_to_column;
		absl::flat_hash_map<std::string, size_t> row_headers;
		absl::flat_hash_map<std::string, size_t> column_headers;
		absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> base_data;
		absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> shadow_data;

		// The following map is only used in meta SLKs and maps the field (+unit/ability ID) to a meta ID
		absl::flat_hash_map<std::string, std::string> meta_map;

		SLK() = default;
		explicit SLK(const fs::path & path, bool local = false);

		void load(const fs::path&, bool local = false);
		void build_meta_map();

		// column_header should be lowercase
		template <typename T = std::string>
		T data(std::string_view column_header, std::string_view row_header) const {
			assert(to_lowercase_copy(column_header) == column_header);

			// Todo, do find() directly as the .contains() and .at().at() do multiple lookups
			if (shadow_data.contains(row_header) && shadow_data.at(row_header).contains(column_header)) {
				if constexpr (std::is_same<T, std::string>()) {
					return shadow_data.at(row_header).at(column_header);
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(shadow_data.at(row_header).at(column_header));
				} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
					return std::stoi(shadow_data.at(row_header).at(column_header));
				}
			}

			//if (const auto found = column_headers.find(std::string(column_header) + ":hd"); found) {
			//	if (!found->first.empty()) {
			//		if constexpr (std::is_same<T, std::string>()) {
			//			return found->first;
			//		} else if constexpr (std::is_same<T, float>()) {
			//			return std::stof(found->first);
			//		} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
			//			return std::stoi(found->first);
			//		}
			//	}
			//}

			// Todo, do .find() directly as the .contains() and .at().at() do multiple lookups
			if (hierarchy.hd && column_headers.contains(std::string(column_header) + ":hd")) {
				std::string hd_data = data(std::string(column_header) + ":hd", row_header);
				if (!hd_data.empty()) {
					if constexpr (std::is_same<T, std::string>()) {
						return hd_data;
					} else if constexpr (std::is_same<T, float>()) {
						return std::stof(hd_data);
					} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
						return std::stoi(hd_data);
					}
				}
			}

			/*if (const auto found = base_data.find(row_header); found) {
				if (const auto found_sub = found->second.find(column_header); found_sub) {
					if constexpr (std::is_same<T, std::string>()) {
						return found_sub->second;
					} else if constexpr (std::is_same<T, float>()) {
						return std::stof(found_sub->second);
					} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
						return std::stoi(found_sub->second);
					}
				}
			}*/


			// Todo, do find() directly as the .contains() and .at().at() do multiple lookups
			if (base_data.contains(row_header) && base_data.at(row_header).contains(column_header)) {
				if constexpr (std::is_same<T, std::string>()) {
					return base_data.at(row_header).at(column_header);
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(base_data.at(row_header).at(column_header));
				} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
					return std::stoi(base_data.at(row_header).at(column_header));
				}
			}

			static_assert("Type not supported. Convert yourself or add conversion here if it makes sense");
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

		void merge(const SLK& slk);
		void merge(const ini::INI& ini, const SLK& meta_slk);
		void substitute(const ini::INI& ini, const std::string& section);
		void copy_row(const std::string_view row_header, const std::string_view new_row_header, bool copy_shadow_data);
		void remove_row(const std::string_view row_header);

		void add_column(const std::string_view column_header);

		/// If the column does not exist then it will be created
		void set_shadow_data(const std::string_view column_header, const std::string_view row_header, std::string data);
		void set_shadow_data(const int column, const int row, std::string data);

		size_t rows() {
			return row_headers.size();
		}

		size_t columns() {
			return column_headers.size();
		}
	};
}
