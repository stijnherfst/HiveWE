#include "stdafx.h"

namespace slk {
	SLK::SLK(const fs::path& path, const bool local) {
		load(path, local);
	}

	void SLK::load(const fs::path& path, const bool local) {
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

		std::string line;

		size_t position = 0;
		size_t length = 0;

		size_t column = 0;
		size_t row = 0;
		size_t max_rows = 0;

		const auto parse_int_part = [&]() {
			position++;
			length = line.find_first_of(';', position) - position;
			position += length + 1;
			return std::stoi(line.substr(position - 1 - length, length));
		};

		if (std::getline(file, line)) {
			if (line.substr(0, 2) != "ID") {
				std::cout << "Invalid SLK file, does not contain \"ID\" as first record" << std::endl;
				return;
			}
		} else {
			return;
		}

		while (std::getline(file, line)) {
			position = 2;

			switch (line.front()) {
				case 'B':
					switch (line[position]) {
						case 'X':
							columns = parse_int_part();
							rows = parse_int_part();
							break;
						case 'Y':
							rows = parse_int_part();
							columns = parse_int_part();
							break;
						default:
							std::cout << "Bad B row in .slk\n";
							return;
					}
					table_data.resize(rows, std::vector<std::string>(columns));
					shadow_data.resize(rows, std::vector<std::string>(columns, shadow_table_empty_identifier));
					break;
				case 'C':
					if (line[position] == 'X') {
						column = parse_int_part() - 1;

						if (line[position] == 'Y') {
							row = parse_int_part() - 1;
						}
					} else {
						row = parse_int_part() - 1;

						if (line[position] == 'X') {
							column = parse_int_part() - 1;
						}
					}
					max_rows = std::max(max_rows, row);

					position++;

					{
						std::string part;
						if (line[position] == '\"') {
							position++;
							part = line.substr(position, line.find('"', position) - position);
						} else {
							part = line.substr(position, line.size() - position - (line.back() == '\r' ? 1 : 0));
						}

						if (part == "-" || part == "_") {
							part = "";
						}

						table_data[row][column] = part;

						if (row == 0) {
							header_to_column.emplace(part, column);
						}

						if (column == 0) {
							header_to_row.emplace(part, row);
						}
					}
					break;
				case 'E':
					goto exitloop;
			}
		}
	exitloop:
		// If there are empty rows at the end
		if (rows > max_rows + 1) {
			rows = max_rows + 1;
			table_data.resize(rows, std::vector<std::string>(columns));
			shadow_data.resize(rows, std::vector<std::string>(columns, shadow_table_empty_identifier));

			for (auto it = header_to_row.begin(); it != header_to_row.end();) {
				if (it->second > max_rows) {
					it = header_to_row.erase(it);
				} else {
					++it;
				}
			}
		}
	}

	void SLK::save(const fs::path& path) const {
		std::ofstream output(path);

		output << "ID;PWXL;N;E\n";
		output << "B;X" << columns << ";Y" << rows << ";D0\n";

		for (auto&&[key, value] : header_to_column) {
			output << "C;X" << value + 1 << ";Y1;K\"" << key << "\"\n";
		}

		for(int i = 1; i < table_data.size(); i++) {
			for (int j = 0; j < table_data[i].size(); j++) {
				output << "C;X" << j + 1 << ";Y" << i + 1 << ";K\"" << table_data[i][j] << "\"\n";
			}
		}
		output << "E";
	}

	bool SLK::row_header_exists(const std::string& row_header) const {
		return header_to_row.contains(row_header);
	}

	/// Merges the data of the files. Any unknown columns are appended
	void SLK::merge(const slk::SLK& slk) {
		for (size_t i = 1; i < slk.columns; i++) {
			header_to_column.emplace(slk.table_data[0][i], columns + i - 1);
		}
		columns = columns + slk.columns - 1;

		for (auto&& i : slk.table_data) {
			const size_t index = header_to_row[i.front()];
			table_data[index].insert(table_data[index].end(), i.begin() + 1, i.end());
		}

		for (size_t i = 0; i < rows; i++) {
			table_data[i].resize(columns);
			shadow_data[i].resize(columns, shadow_table_empty_identifier);
		}
	}

	/// Merges the data of the files. INI sections are matched to row keys and INI keys are matched to column keys.
	/// If an unknown section key is encountered then that section is skipped
	/// If an unknown column key is encountered then the column is added
	void SLK::merge(const ini::INI & ini) {
		for (auto&& [section_key, section_value] : ini.ini_data) {
			// If id does not exist
			if (!header_to_row.contains(section_key)) {
				continue;
			}
			for (auto&& [key, value] : section_value) {
				if (!header_to_column.contains(key)) {
					add_column(key);
				}
				table_data[header_to_row[section_key]][header_to_column[key]] = std::accumulate(value.begin(), value.end(), ""s);;
			}
		}
	}

	/// Substitutes the data of the slk with data from the INI based on a certain section key.
	/// The keys of the section are matched with all the cells in the table and if they match will replace the value
	void SLK::substitute(const ini::INI & ini, const std::string& section) {
		for (auto&& i : table_data) {
			for (auto&& j : i) {
				std::string data = ini.data(section, j);
				if (!data.empty()) {
					j = data;
				}
			}
		}
	}

	/// Copies the row with header row_header to a new line with the new header as new_row_header
	void SLK::copy_row(const std::string& row_header, const std::string& new_row_header) {
		if (!header_to_row.contains(row_header)) {
			std::cout << "Unknown row header: " << row_header << "\n";
			return;
		}

		const size_t row = header_to_row[row_header];

		table_data.emplace_back(table_data[row]);
		table_data[table_data.size() - 1][0] = new_row_header;
		shadow_data.emplace_back(std::vector<std::string>(columns, shadow_table_empty_identifier));
		header_to_row.emplace(new_row_header, table_data.size() - 1);
		rows++;
	}

	void SLK::add_column(const std::string& header) {
		columns += 1;
		for(auto&& i : table_data) {
			i.resize(columns);
		}
		for (auto&& i : shadow_data) {
			i.resize(columns, shadow_table_empty_identifier);
		}
		header_to_column.emplace(header, columns - 1);
	}

	void SLK::set_shadow_data(const std::string& column_header, const std::string& row_header, const std::string& data) {
		if (!header_to_column.contains(column_header)) {
			std::cout << "Unknown column header: " << column_header << "\n";
			return;
		}

		if (!header_to_row.contains(row_header)) {
			std::cout << "Unknown row header: " << row_header << "\n";
			return;
		}

		const size_t column = header_to_column[column_header];
		const size_t row = header_to_row[row_header];

		shadow_data[row][column] = data;
	}
}