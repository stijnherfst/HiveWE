#pragma once

namespace slk {
	class SLK {
	public:
		size_t rows;
		size_t columns;

		std::unordered_map<std::string, size_t> header_to_column;
		std::unordered_map<std::string, size_t> header_to_row;

		std::vector<std::vector<std::string>> table_data;

		SLK() {}
		SLK(std::string path, bool local = false);

		void load(std::string path, bool local = false);

		std::string data(std::string column_header, size_t row);
		std::string data(std::string column_header, std::string row_header);
	};
}