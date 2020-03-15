#pragma once

#include "INI.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include "Hierarchy.h"

namespace slk {
	class SLK {
		std::vector<std::vector<std::string>> table_data;
		std::vector<std::vector<std::string>> shadow_data;

		constexpr static char shadow_table_empty_identifier[] = "dezecelisleeg"; // not a nice way to do this
	public:

		std::unordered_map<std::string, size_t> header_to_column;
		std::unordered_map<std::string, size_t> header_to_row;
		size_t rows = 0;
		size_t columns = 0;


		SLK() = default;
		explicit SLK(const fs::path& path, bool local = false);

		void load(const fs::path&, bool local = false);
		void save(const fs::path& path) const;

		// column_header should be lowercase
		template<typename T = std::string>
		T data(const std::string& column_header, size_t row) const {
			if (!header_to_column.contains(column_header)) {
				return T();
			}

			const size_t column = header_to_column.at(column_header);

			if (row >= rows) {
				std::cout << "Reading invalid row: " << row + 1 << "/" << rows << "\n";
				return T();
			}

			if (shadow_data[row][column] != shadow_table_empty_identifier) {
				if constexpr (std::is_same<T, std::string>()) {
					return shadow_data[row][column];
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(shadow_data[row][column]);
				} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
					return std::stoi(shadow_data[row][column]);
				}
			}

			if (hierarchy.hd && header_to_column.contains(column_header + ":hd")) {
				std::string hd_data = data(column_header + ":hd", row);
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

			if constexpr (std::is_same<T, std::string>()) {
				return table_data[row][column];
			} else if constexpr (std::is_same<T, float>()) {
				return std::stof(table_data[row][column]);
			} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
				return std::stoi(table_data[row][column]);
			}

			static_assert("Type not supported. Convert yourself or add conversion here if it makes sense");
		}

		template<typename T = std::string>
		T data(size_t column, size_t row) const {
			if (row >= rows) {
				std::cout << "Reading invalid row: " << row + 1 << "/" << rows << "\n";
				return T();
			}

			if (column >= columns) {
				std::cout << "Reading invalid column: " << column + 1 << "/" << columns << "\n";
				return T();
			}

			if (shadow_data[row][column] != shadow_table_empty_identifier) {
				if constexpr (std::is_same<T, std::string>()) {
					return shadow_data[row][column];
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(shadow_data[row][column]);
				} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
					return std::stoi(shadow_data[row][column]);
				}
			}

			if constexpr (std::is_same<T, std::string>()) {
				return table_data[row][column];
			} else if constexpr (std::is_same<T, float>()) {
				return std::stof(table_data[row][column]);
			} else if constexpr (std::is_same<T, int>() || std::is_same<T, bool>()) {
				return std::stoi(table_data[row][column]);
			}

			static_assert("Type not supported. Convert yourself or add conversion here if it makes sense");
		}

		// column_header should be lowercase
		template<typename T = std::string>
		T data(const std::string& column_header, const std::string& row_header) const {
			if (!header_to_row.contains(row_header)) {
				return T();
			}

			const size_t row = header_to_row.at(row_header);

			return data<T>(column_header, row);
		}

		bool row_header_exists(const std::string& row_header) const;

		void merge(const SLK& slk);
		void merge(const ini::INI& ini);
		void substitute(const ini::INI & ini, const std::string& section);
		void copy_row(const std::string& row_header, const std::string& new_row_header);

		// header should be lowercase
		template<typename T>
		void add_column(const std::string& header, T default_value) {
			columns += 1;
			for (auto&& i : table_data) {
				if constexpr (std::is_same_v<T, std::string>()) {
					i.resize(columns, default_value);
				} else {
					i.resize(columns, std::to_string(default_value));
				}
			}
			for (auto&& i : shadow_data) {
				if constexpr (std::is_same_v<T, std::string>()) {
					i.resize(columns, default_value);
				} else {
					i.resize(columns, std::to_string(default_value));
				}
			}
			header_to_column.emplace(header, columns - 1);
		}

		void add_column(const std::string& header);

		void set_shadow_data(const std::string& column_header, const std::string& row_header, const std::string& data);
		void set_shadow_data(const int column, const int row, const std::string& data);
	};
}