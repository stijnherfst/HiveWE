#pragma once

#include "INI.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include "Hierarchy.h"

#include <absl/container/flat_hash_map.h>

namespace slk {
	class SLK2 {

	public:
		absl::flat_hash_map<size_t, std::string> index_to_row;
		absl::flat_hash_map<size_t, std::string> index_to_column;
		absl::flat_hash_map<std::string, size_t> row_headers;
		absl::flat_hash_map<std::string, size_t> column_headers;
		absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> base_data;
		absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, std::string>> shadow_data;

		SLK2() = default;
		explicit SLK2(const fs::path & path, bool local = false);

		void load(const fs::path&, bool local = false);

		// column_header should be lowercase
		template <typename T = std::string>
		T data(std::string_view column_header, std::string_view row_header) const {
			if (shadow_data.contains(row_header) && shadow_data.at(row_header).contains(column_header)) {
				if constexpr (std::is_same<T, std::string>()) {
					return shadow_data.at(row_header).at(column_header);
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(shadow_data.at(row_header).at(column_header));
				} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
					return std::stoi(shadow_data.at(row_header).at(column_header));
				}
			}

			//if (hierarchy.hd && header_to_column.contains(column_header + ":hd")) {
			//	std::string hd_data = data(column_header + ":hd", row);
			//	if (!hd_data.empty()) {
			//		if constexpr (std::is_same<T, std::string>()) {
			//			return hd_data;
			//		} else if constexpr (std::is_same<T, float>()) {
			//			return std::stof(hd_data);
			//		} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
			//			return std::stoi(hd_data);
			//		}
			//	}
			//}

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
		template <typename T = std::string>
		T data(const std::string_view column_header, size_t row) const {
			if (row >= index_to_row.size()) {
				return T();
			}

			return data(column_header, index_to_row.at(row));
		}

		// Gets the data by first checking the shadow table and then checking the base table
		template <typename T = std::string>
		T data(size_t column, size_t row) const {
			if (row >= index_to_row.size()) {
				return T();
			}

			if (column >= index_to_column.size()) {
				return T();
			}

			return data(index_to_column.at(column), index_to_row.at(row));
		}

		void merge(const SLK2& slk);
		void merge(const ini::INI& ini);
		void substitute(const ini::INI& ini, const std::string& section);
		void copy_row(const std::string_view row_header, const std::string_view new_row_header);

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