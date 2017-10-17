#include "stdafx.h"

namespace slk {
	SLK::SLK(std::string path) {
		load(path);
	}

	void SLK::load(std::string path) {
		std::stringstream file;
		file << hierarchy.open_file(path).data();

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
			//std::cout << line << std::endl;
			if (line.back() == '\r') {
				line.erase(line.end() - 1);
			}

			auto parts = split(line, ';');
			if (!parts.size()) {
				std::cout << "Invalid SLK file" << std::endl;
				break;
			}

			if (parts[0] == "B") {
				if (parts.size() < 4) {
					std::cout << "Invalid B record: " << line << std::endl;
					break;
				}
				parts[1].erase(parts[1].begin());
				parts[2].erase(parts[2].begin());
				int columns = std::stoi(parts[1]);
				int rows = std::stoi(parts[2]);
				data.resize(columns, std::vector<std::string>(rows));
				continue;
			} else if (parts[0] == "C") {
				if (parts.size() < 3) {
					std::cout << "Invalid C record: " << line << std::endl;
					break;
				}

				int column = 0;
				for (auto&& part : parts) {
					switch (part.front()) {
						case 'X':
							part.erase(part.begin());
							column = std::stoi(part);
							break;
						case 'Y':
							part.erase(part.begin());
							row = std::stoi(part);
							break;
						case 'K':
							part.erase(part.begin());
							if (part.front() == '\"') {
								data[column - 1][row - 1] = part.substr(1, part.size() - 2);
							} else {
								data[column - 1][row - 1] = part;
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
}