#include "stdafx.h"

void TriggerStrings::load(BinaryReader& reader) {
	std::stringstream file;
	file << reader.buffer.data();

	std::string key;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line.substr(0, 2) == "//") {
			continue;
		}
		if (line.back() == '\r') {
			line.resize(line.size() - 1);
		}

		if (line.empty()) {
			continue;
		}

		line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return c == '\r'; }), line.end());

		if (line.front() == '{') {
			std::string value;
			while (std::getline(file, line) && !line.empty() && line.front() != '}') {
				if (line.back() == '\r') {
					line.resize(line.size() - 1);
				}
				value += line;
			}
			strings.emplace(key, value);
		} else {
			auto found = line.find(' ') + 1;
			int padsize = std::max(0, 3 - ((int)line.size() - (int)found));
			key = "TRIGSTR_" + std::string(padsize, '0') + line.substr(found);
		}
	}
}

void TriggerStrings::save() const {
	BinaryWriter writer;

	std::stringstream file;
	for (auto&& [key, value] : strings) {
		auto found = key.find('_') + 1;
		std::string final_string = "STRING " + key.substr(found);
		final_string.erase(std::remove_if(final_string.begin(), final_string.end(), [](char c) { return c == '0'; }), final_string.end());
		
		writer.write_string(final_string);
		writer.write_string("\n{\n");
		writer.write_string(value);
		writer.write_string("\n}\n\n");
	}

	HANDLE handle;
	const bool success = SFileCreateFile(hierarchy.map.handle, "war3map.wts", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	SFileFinishFile(handle);
}