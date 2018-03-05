#include "stdafx.h"

// Don't look at this code
// It's not pretty

namespace slk {
	SLK::SLK(const fs::path& path, bool local) {
		load(path, local);
	}

	void SLK::load(const fs::path& path, bool local) {
		std::stringstream file;
		if (local) {
			std::ifstream stream(path);
			if (stream) {
				file << stream.rdbuf();
			}
			stream.close();
		} else {
			file << hierarchy.open_file(path).buffer.data();
		}

		int row = 1;

		std::string line;
		if (std::getline(file, line)) {
			auto parts = split(line, ';');

			const std::string id = parts[0];
			if (id != "ID") {
				std::cout << "Invalid SLK file, does not contain \"ID\" as first record" << std::endl;
				return;
			}
		} else {
			return;
		}

		while (std::getline(file, line)) {
			if (line.back() == '\r') {
				line.erase(line.end() - 1);
			}

			auto parts = split(line, ';');
			if (parts.empty()) {
				std::cout << "Invalid SLK file" << std::endl;
				break;
			}

			if (parts.front() == "B") {
				if (parts.size() < 3) {
					std::cout << "Invalid B record: " << line << std::endl;
					break;
				}
				for (auto&& part : parts) {
					switch (part.front()) {
						case 'X':
							part.erase(part.begin());
							columns = std::stoi(part);
							break;
						case 'Y':
							part.erase(part.begin());
							rows = std::stoi(part);
							break;
					}
				}
				table_data.resize(rows, std::vector<std::string>(columns));
				shadow_data.resize(rows, std::vector<std::string>(columns));
			} else if (parts.front() == "C") {
				if (parts.size() < 3) {
					std::cout << "Invalid C record: " << line << std::endl;
					break;
				}

				int column = 0;
				for (auto&& part : parts) {
					const char front = part.front();
					part.erase(part.begin());
					switch (front) {
						case 'X':
							column = std::stoi(part) - 1;
							break;
						case 'Y':
							row = std::stoi(part) - 1;
							break;
						case 'K':
							if (part.front() == '\"') {
								if (row == 0) {
									header_to_column.emplace(part.substr(1, part.size() - 2), column);
								}
								if (column == 0) {
									header_to_row.emplace(part.substr(1, part.size() - 2), row);
								}
								table_data[row][column] = part.substr(1, part.size() - 2);
							} else {
								if (row == 0) {
									header_to_column.emplace(part, column);
								}
								if (column == 0) {
									header_to_row.emplace(part, row);
								}
								table_data[row][column] = part;
							}
							break;
					}
				}
			} else if (line == "E") {
				break;
			} else {
				std::cout << "Invalid or unknown record: " << line << std::endl;
			}
		}
	}

	std::string SLK::data(const std::string& column_header, const size_t row) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			return "";
		}

		const size_t column = header_to_column[column_header];

		if (row >= rows) {
			std::cout << "Reading invalid row: " <<  row + 1 << "/" << rows;
			return "";
		}

		if (!shadow_data[row][column].empty()) {
			return shadow_data[row][column];
		}

		return table_data[row][column];
	}

	std::string SLK::data(const std::string& column_header, const std::string& row_header) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			return "";
		}

		if (header_to_row.find(row_header) == header_to_row.end()) {
			return "";
		}

		const size_t column = header_to_column[column_header];
		const size_t row = header_to_row[row_header];

		if (!shadow_data[row][column].empty()) {
			return shadow_data[row][column];
		}

		return table_data[row][column];
	}

	void SLK::merge(const slk::SLK& slk) {
		for (size_t i = 1; i < slk.columns; i++) {
			header_to_column.emplace(slk.table_data[0][i], columns + i - 1);
		}
		columns = columns + slk.columns - 1;

		for (auto&& i : slk.table_data) {
			const int index = header_to_row[i.front()];
			table_data[index].insert(table_data[index].end(), i.begin() + 1, i.end());
		}

		for (size_t i = 0; i < rows; i++) {
			table_data[i].resize(columns);
			shadow_data[i].resize(columns);
		}
	}

	void SLK::merge(const ini::INI & ini) {
		for (auto&& [section_key, section_value] : ini.ini_data) {
			// If id does not exist
			if (header_to_row.find(section_key) == header_to_row.end()) {
				continue;
			}
			for (auto&& [key, value] : section_value) {
				if (header_to_column.find(key) == header_to_column.end()) {
					add_column(key);
				}
				table_data[header_to_row[section_key]][header_to_column[key]] = value;
			}
		}
	}

	void SLK::copy_row(const std::string& row_header, const std::string& new_row_header) {
		if (header_to_row.find(row_header) == header_to_row.end()) {
			std::cout << "Uknown row header: " << row_header << "\n";
			return;
		}

		const size_t row = header_to_row[row_header];

		table_data.emplace_back(table_data[row]);
		table_data[table_data.size() - 1][0] = new_row_header;
		shadow_data.emplace_back(std::vector<std::string>(columns));
		header_to_row.emplace(new_row_header, table_data.size() - 1);
	}

	void SLK::add_column(const std::string& header) {
		columns += 1;
		for(auto&& i : table_data) {
			i.resize(columns);
		}
		for (auto&& i : shadow_data) {
			i.resize(columns);
		}
		header_to_column.emplace(header, columns - 1);
	}

	void SLK::set_shadow_data(const std::string& column_header, const std::string& row_header, const std::string& data) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			std::cout << "Uknown column header: " << column_header << "\n";
			return;
		}

		if (header_to_row.find(row_header) == header_to_row.end()) {
			std::cout << "Uknown row header: " << row_header << "\n";
			return;
		}

		const size_t column = header_to_column[column_header];
		const size_t row = header_to_row[row_header];

		shadow_data[row][column] = data;
	}
}