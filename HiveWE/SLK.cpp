#include "stdafx.h"

// Don't look at this code
// It's not pretty

namespace slk {
	SLK::SLK(std::string path, bool local) {
		load(path, local);
	}

	void SLK::load(std::string path, bool local) {
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

			std::string id = parts[0];
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
			if (!parts.size()) {
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
				continue;
			} else if (parts.front() == "C") {
				if (parts.size() < 3) {
					std::cout << "Invalid C record: " << line << std::endl;
					break;
				}

				int column = 0;
				for (auto&& part : parts) {
					char front = part.front();
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
				continue;
			} else if (line == "E") {
				break;
			} else {
				std::cout << "Invalid or unknown record: " << line << std::endl;
				continue;
			}
		}
	}

	std::string SLK::data(std::string column_header, size_t row) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			return "";
		}

		size_t column = header_to_column[column_header];

		if (row >= rows) {
			std::cout << "Reading invalid row: " <<  row + 1 << "/" << rows;
			return "";
		}

		if (shadow_data[row][column] != "") {
			return shadow_data[row][column];
		}

		return table_data[row][column];
	}

	std::string SLK::data(std::string column_header, std::string row_header) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			return "";
		}

		if (header_to_row.find(row_header) == header_to_row.end()) {
			return "";
		}

		size_t column = header_to_column[column_header];
		size_t row = header_to_row[row_header];

		if (shadow_data[row][column] != "") {
			return shadow_data[row][column];
		}

		return table_data[row][column];
	}

	void SLK::copy_row(std::string row_header, std::string new_row_header) {
		if (header_to_row.find(row_header) == header_to_row.end()) {
			std::cout << "Uknown row header: " << row_header << "\n";
			return;
		}

		size_t row = header_to_row[row_header];

		table_data.emplace_back(table_data[row]);
		table_data[table_data.size() - 1][0] = new_row_header;
		shadow_data.push_back(std::vector<std::string>(columns));
		header_to_row.emplace(new_row_header, table_data.size() - 1);
	}

	void SLK::set_shadow_data(std::string column_header, std::string row_header, std::string data) {
		if (header_to_column.find(column_header) == header_to_column.end()) {
			std::cout << "Uknown column header: " << column_header << "\n";
			return;
		}

		if (header_to_row.find(row_header) == header_to_row.end()) {
			std::cout << "Uknown row header: " << row_header << "\n";
			return;
		}

		size_t column = header_to_column[column_header];
		size_t row = header_to_row[row_header];

		shadow_data[row][column] = data;
	}
}