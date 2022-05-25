module;

#include <filesystem>
#include <unordered_set>

export module Imports;

import BinaryWriter;
import Hierarchy;
namespace fs = std::filesystem;

// HiveWE does not use the war3map.imp file at all, but the old WE does
// That's why we write the file at save time
export class Imports {
  public:

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

	/// Requires the filesystem_path for the map to make the saved paths lexically relative
	void save(fs::path filesystem_path) const {
		BinaryWriter writer;

		writer.write<uint32_t>(1);

		int count = 0;
		for (const auto& i : fs::recursive_directory_iterator(filesystem_path)) {
			if (i.is_regular_file()) {
				std::string file_name = i.path().filename().string();
				if (blacklist.contains(file_name)) {
					continue;
				}
				count++;
			}
		}
		writer.write<uint32_t>(count);

		for (const auto& i : fs::recursive_directory_iterator(filesystem_path)) {
			if (i.is_regular_file()) {
				std::string path = i.path().lexically_relative(filesystem_path).string();
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
};