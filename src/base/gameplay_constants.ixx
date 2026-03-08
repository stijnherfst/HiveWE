export module GameplayConstants;

import std;
import INI;
import SLK;
import Hierarchy;
import Globals;
import Utilities;
import "absl/strings/str_join.h";
import "absl/strings/str_split.h";

namespace fs = std::filesystem;

export constexpr const char* constants_row_key = "yeet";

export class GameplayConstants {
  public:
	slk::SLK metadata;
	slk::SLK data;

	void load() {
		metadata = slk::SLK("Units/MiscMetaData.slk");
		metadata.substitute(world_edit_strings, "WorldEditStrings");
		metadata.build_meta_map();

		// While there is a gameplay constants meta SLK there is no data SLK,
		// So we create it from the two gameplay constants INI files
		// No idea why it is split into two
		ini::INI defaults = ini::INI();
		defaults.load("Units/MiscData.txt");
		defaults.load("Units/MiscGame.txt");

		data.base_data[constants_row_key] = {};
		size_t index = data.row_headers.size();
		data.row_headers.emplace(constants_row_key, index);
		data.index_to_row[index] = constants_row_key;

		for (const auto& [key, values] : defaults.ini_data["Misc"]) {
			const auto lowercase_key = to_lowercase_copy(key);
			if (!metadata.meta_map.contains(lowercase_key)) {
				std::println("Warning: Gameplay meta data doesn't contain the {} column", key);
			}

			data.add_column( lowercase_key);
			data.base_data[constants_row_key][lowercase_key] = absl::StrJoin(values, ",");
		}

		if (hierarchy.map_file_exists("war3mapMisc.txt")) {
			// Use INI loader in local mode pointing to the map file on disk
			const ini::INI overrides = ini::INI(hierarchy.map_directory / "war3mapMisc.txt", true);

			for (const auto& [key, values] : overrides.ini_data.at("Misc")) {
				const auto lowercase_key = to_lowercase_copy(key);
				if (!metadata.meta_map.contains(lowercase_key)) {
					std::println("Warning: Gameplay meta data doesn't contain the {} column", key);
				}

				data.set_shadow_data(lowercase_key, constants_row_key, absl::StrJoin(values, ","));
			}
		}
	}

	void save() {
		ini::INI war3mapMisc;

		war3mapMisc.ini_data["Misc"] = {};
		for (const auto& [key, value] : data.shadow_data[constants_row_key]) {
			war3mapMisc.ini_data["Misc"][key] = absl::StrSplit(value, ',', absl::SkipEmpty());
		}
		war3mapMisc.save(hierarchy.map_directory / "war3mapMisc.txt");
	}
};
