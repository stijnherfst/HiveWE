export module MapData;

import std;
import SLK;
import PathingMap;
import Hierarchy;
import UnorderedMap;

export class TerrainTexture {
  public:
	/// String ID of the terrain texture. E.g. 'Ldrt'
	std::string id;

	/// Display name of the terrain texture. E.g. "Dark Grass"
	std::string name;

	/// Texture file path
	std::string file_path;

	/// Base game pathing flags for the terrain texture
	uint8_t base_pathing;

	/// User-defined pathing override. If set, replaces base_pathing
	std::optional<uint8_t> override_pathing;

	/// Optional, the cliff type which the terrain texture is associated with
	std::optional<std::string> cliff_type_id;

	uint8_t get_tile_pathing() const {
		return override_pathing.value_or(base_pathing);
	}
};

export class CliffType {
  public:
	/// String id of the cliff. E.g. 'CLdi'
	std::string id;

	/// Display name of the cliff type.
	std::string name;

	/// String id of the ground tile underneath the cliff
	std::string ground_tile;

	/// File path of the cliff texture.
	std::string file_path;

	/// Folder containing the cliff models. Either "Cliffs" or "CityCliffs"
	std::string cliff_model_dir;

	/// Folder containing the ramp models. Either "CliffTrans" or "CityCliffTrans"
	std::string ramp_model_dir;
};

/// Class which holds all map data, including HiveWE specific data.
export class MapData {
  public:
	/// Returns the path to the HiveWE data folder inside the map directory
	static std::filesystem::path hive_data_folder() {
		return hierarchy.map_directory / "hiveWE";
	}

	/// Returns the path to the terrain pathing override file inside the map directory
	static std::filesystem::path terrain_pathing_file() {
		return hive_data_folder() / "terrain_pathing.json";
	}

	/// Returns a pointer to the terrain texture with the given ID, or nullptr if not found
	const TerrainTexture* terrain_texture(const std::string& id) const {
		const auto it = m_terrain_textures.find(id);
		return it != m_terrain_textures.end() ? &it->second : nullptr;
	}

	/// Returns a mutable pointer to the terrain texture with the given ID, or nullptr if not found
	TerrainTexture* terrain_texture(const std::string& id) {
		const auto it = m_terrain_textures.find(id);
		return it != m_terrain_textures.end() ? &it->second : nullptr;
	}

	/// Returns a pointer to the cliff type with the given ID, or nullptr if not found
	const CliffType* cliff_type(const std::string& id) const {
		const auto it = m_cliff_types.find(id);
		return it != m_cliff_types.end() ? &it->second : nullptr;
	}

	/// Returns all terrain textures
	const hive::unordered_map<std::string, TerrainTexture>& terrain_textures() const {
		return m_terrain_textures;
	}

	/// Returns all cliff types
	const hive::unordered_map<std::string, CliffType>& cliff_types() const {
		return m_cliff_types;
	}

	void load() {
		load_terrain_data();
	}

	void save() const {
		save_terrain_data();
	}

  private:
	/// All avilible terrain textures and cliff types in the game
	hive::unordered_map<std::string, TerrainTexture> m_terrain_textures;
	hive::unordered_map<std::string, CliffType> m_cliff_types;

	/// Loads terrain and cliff .slk files and the terrain_pathing_file.
	void load_terrain_data();

	/// Saves the user defined terrain pathing as a .json. This is a HiveWE exclusive feature.
	/// It will not be read by vanilla WE.
	void save_terrain_data() const;
};
