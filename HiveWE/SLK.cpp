#include "stdafx.h"

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
			file << hierarchy.open_file(path).data();
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
				table_data.resize(columns, std::vector<std::string>(rows));
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
							column = std::stoi(part);
							break;
						case 'Y':
							row = std::stoi(part);
							break;
						case 'K':
							if (part.front() == '\"') {
								if (row == 1) {
									header_to_column.emplace(part.substr(1, part.size() - 2), column - 1);
								}
								if (column == 1) {
									header_to_row.emplace(part.substr(1, part.size() - 2), row - 1);
								}
								table_data[column - 1][row - 1] = part.substr(1, part.size() - 2);
							} else {
								if (row == 1) {
									header_to_column.emplace(part, column - 1);
								}
								if (column == 1) {
									header_to_row.emplace(part, row - 1);
								}
								table_data[column - 1][row - 1] = part;
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
		return table_data[header_to_column[column_header]][row];
	}

	std::string SLK::data(std::string column_header, std::string row_header) {
		return table_data[header_to_column[column_header]][header_to_row[row_header]];
	}
}