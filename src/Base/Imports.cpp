#include <iostream>
#include <functional>
#include <filesystem>

#include <QString>

#include "Imports.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Hierarchy.h"
#include "ResourceManager.h"

#include "HiveWE.h"

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

std::unordered_set<std::string> blacklist = {
	"conversation.json",
	"war3mapUnits.doo",
	"war3map.doo",
	"war3map.imp",
	"war3map.j",
	"war3map.mmp",
	"war3map.shd",
	"war3map.w3a",
	"war3map.w3b",
	"war3map.w3c",
	"war3map.w3d",
	"war3map.w3e",
	"war3map.w3h",
	"war3map.w3i",
	"war3map.w3q",
	"war3map.w3r",
	"war3map.w3t",
	"war3map.w3u",
	"war3map.wct",
	"war3map.wpm",
	"war3map.wtg",
	"war3map.wts",
	"war3map.w3s",
	"war3mapMap.blp",
	"war3mapExtra.txt",
	"war3mapMisc.txt",
	"war3mapSkin.txt"
};

void Imports::save() const {
	BinaryWriter writer;
		
	writer.write<uint32_t>(1);

	int count = 0;
	for (const auto& i : fs::recursive_directory_iterator(map->filesystem_path)) {
		if (i.is_regular_file()) {
			std::string file_name = i.path().filename().string();
			if (blacklist.contains(file_name)) {
				continue;
			}
			count++;
		}
	}
	writer.write<uint32_t>(count);

	for (const auto& i : fs::recursive_directory_iterator(map->filesystem_path)) {
		if (i.is_regular_file()) {
			std::string path = i.path().lexically_relative(map->filesystem_path).string();
			std::string file_name = i.path().filename().string();
			if (blacklist.contains(file_name)) {
				continue;
			}

			writer.write<uint8_t>(13);
			writer.write_c_string(path);
		}
	}

	hierarchy.map_file_write("war3map.imp", writer.buffer);
}