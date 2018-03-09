#pragma once

namespace slk {
	class SLK {
	public:
		size_t rows = 0;
		size_t columns = 0;

		std::unordered_map<std::string, size_t> header_to_column;
		std::unordered_map<std::string, size_t> header_to_row;

		std::vector<std::vector<std::string>> table_data;
		std::vector<std::vector<std::string>> shadow_data;

		SLK() = default;
		explicit SLK(const fs::path& path, bool local = false);

		void load(const fs::path&, bool local = false);

		std::string data(const std::string& column_header, size_t row);
		std::string data(const std::string& column_header, const std::string& row_header);


		void merge(const SLK& slk);
		void merge(const ini::INI& ini);
		void copy_row(const std::string& row_header, const std::string& new_row_header);
		void add_column(const std::string& header);

		void set_shadow_data(const std::string& column_header, const std::string& row_header, const std::string& data);
	};
}
