#pragma once

namespace slk {
	class SLK {
	public:
		size_t rows;
		size_t columns;

		std::unordered_map<std::string, size_t> header_to_column;
		std::unordered_map<std::string, size_t> header_to_row;

		std::vector<std::vector<std::string>> table_data;
		std::vector<std::vector<std::string>> shadow_data;

		SLK() = default;
		SLK(fs::path path, bool local = false);

		void load(fs::path, bool local = false);

		std::string data(std::string column_header, size_t row);
		std::string data(std::string column_header, std::string row_header);

		void copy_row(std::string row_header, std::string new_row_header);

		void set_shadow_data(std::string column_header, std::string row_header, std::string data);
	};
}