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
		SLK(const fs::path& path, bool local = false);

		void load(const fs::path&, bool local = false);

		std::string data(std::string column_header, size_t row);
		std::string data(std::string column_header, std::string row_header);

		void merge(const slk::SLK& slk);
		void copy_row(const std::string& row_header, const std::string& new_row_header);

		void set_shadow_data(const std::string& column_header, const std::string& row_header, const std::string& data);
	};
}