#include "stdafx.h"

#include <QFile>

void TriggerStrings::load(BinaryReader& reader) {
	std::stringstream file;
	file << reader.buffer.data();

	std::string key;
	std::string line;
	while (std::getline(file, line)) {
		// Get rid of \r
		line.resize(line.size() - 1);
		if (line.empty() || line.substr(0, 2) == "//") {
			continue;
		}
		if (line.front() == '{') {
			std::string value;
			while (std::getline(file, line) && line.front() != '}') {
				// Get rid of \r
				line.resize(line.size() - 1);
				value += line;
			}
			strings.emplace(key, value);
		} else {
			auto found = line.find(' ') + 1;
			int padsize = std::max(0, 3 - ((int)line.size() - (int)found));
			key = "TRIGSTR_" + std::string(padsize, '0') + line.substr(found);
		}
	}
	;
}

void TriggerStrings::save() const {

}