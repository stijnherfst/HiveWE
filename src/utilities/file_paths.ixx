export module Paths;

import <filesystem>;

namespace fs = std::filesystem;

namespace paths {

	/// Returns the path to the HiveWE data folder inside the map directory
	export fs::path hive_data_folder(const fs::path& map_directory) {
		return map_directory / "hiveWE";
	}

	/// Returns the path to the terrain pathing override file
	export fs::path terrain_pathing_file(const fs::path& map_directory) {
		return hive_data_folder(map_directory) / "terrain_pathing.json";
	}

	/// Returns the path to the map info file
	export fs::path map_info_extras_file(const fs::path& map_directory) {
		return hive_data_folder(map_directory) / "map_info_extras.json";
	}

} // namespace paths
