module;

#include <QMessageBox>

export module Tileset;

import std;
import SLK;
import PathingMap;
import Hierarchy;
import UnorderedMap;
import Paths;
import Globals;
import <nlohmann/json.hpp>;
import "glm/glm.hpp";

export class TerrainTexture {
  public:
	std::string id;

	/// Display name of the terrain texture. E.g. "Dark Grass"
	/// Takes editor localisation into account
	std::string name;

	/// Texture file path
	/// Handles both classic and reforged graphics
	std::string file_path;

	/// Base game pathing flags for the terrain texture
	uint8_t base_pathing;

	/// User-defined pathing override. If set, replaces base_pathing
	std::optional<uint8_t> override_pathing;

	/// Optional, the cliff type which the terrain texture is associated with
	std::optional<std::string> cliff_type_id;

	[[nodiscard]]
	uint8_t get_tile_pathing() const {
		return override_pathing.value_or(base_pathing);
	}
};

export class CliffType {
  public:
	std::string id;

	/// Display name of the cliff type
	/// Takes editor localisation into account
	std::string name;

	/// String id of the ground tile underneath the cliff
	std::string ground_tile;

	/// File path of the cliff texture
	/// Handles both classic and reforged graphics
	std::string file_path;

	/// Folder containing the cliff models
	/// Either "Cliffs" or "CityCliffs"
	std::string cliff_model_dir;

	/// Folder containing the ramp models
	/// Either "CliffTrans" or "CityCliffTrans"
	std::string ramp_model_dir;
};

export class Tileset {
  public:
	char id;

	/// Name of the tileset
	/// Takes editor localisation into account
	std::string name;

	/// Path to the blight texture used by the tilesets
	/// Handles both classic and reforged graphics
	std::string blight_texture;

	std::string terrain_dnc = "Environment/DNC/DNCLordaeron/DNCLordaeronTerrain/DNCLordaeronTerrain.mdl";
	std::string unit_dnc = "Environment/DNC/DNCLordaeron/DNCLordaeronUnit/DNCLordaeronUnit.mdl";

	std::string day_ambience_sound;
	std::string night_ambience_sound;
	std::string sound_environment;

	std::string water_id;

	std::vector<std::string> terrain_textures;

	/// Partial path to the water texture used by the tileset
	std::string water_texture;

	float water_offset;
	int water_textures_nr;
	int water_animation_rate;

	/// Shallow water color in RGBA format
	glm::vec4 shallow_color_min;

	/// Shallow water color in RGBA format
	glm::vec4 shallow_color_max;

	/// Deep water color in RGBA format
	glm::vec4 deep_color_min;

	/// Deep water color in RGBA format
	glm::vec4 deep_color_max;
};

/// Holds all tileset related data, including terrain textures and cliff types
export class TilesetData {
  public:
	/// Returns a pointer to the terrain texture with the given ID, or nullptr if not found
	[[nodiscard]]
	const TerrainTexture* terrain_texture(const std::string_view& id) const {
		const auto it = terrain_texture_map.find(id);
		return it != terrain_texture_map.end() ? &it->second : nullptr;
	}

	/// Returns a mutable pointer to the terrain texture with the given ID, or nullptr if not found
	[[nodiscard]]
	TerrainTexture* terrain_texture(const std::string_view& id) {
		const auto it = terrain_texture_map.find(id);
		return it != terrain_texture_map.end() ? &it->second : nullptr;
	}

	/// Returns a pointer to the cliff type with the given ID, or nullptr if not found
	[[nodiscard]]
	const CliffType* cliff_type(const std::string_view& id) const {
		const auto it = cliff_type_map.find(id);
		return it != cliff_type_map.end() ? &it->second : nullptr;
	}

	/// Returns a pointer to the tileset type with the given ID, or nullptr if not found
	[[nodiscard]]
	const Tileset* tileset(const char& id) const {
		const auto it = tileset_map.find(id);
		return it != tileset_map.end() ? &it->second : nullptr;
	}

	/// Returns all terrain textures
	[[nodiscard]]
	const hive::unordered_map<std::string, TerrainTexture>& terrain_textures() const {
		return terrain_texture_map;
	}

	/// Returns all cliff types
	[[nodiscard]]
	const hive::unordered_map<std::string, CliffType>& cliff_types() const {
		return cliff_type_map;
	}

	/// Returns all tilesets
	[[nodiscard]]
	const hive::unordered_map<char, Tileset>& tilesets() const {
		return tileset_map;
	}

	void load() {
		// load .slk files
		// the game does not let the map change those, so we will load them directly from casc
		// or from the game folder if local data is enabled
		Hierarchy::FileSource flags = Hierarchy::FileSource::overrides | Hierarchy::FileSource::local_files | Hierarchy::FileSource::casc;
		slk::SLK terrain_slk("TerrainArt/Terrain.slk", flags);
		slk::SLK cliff_slk("TerrainArt/CliffTypes.slk", flags);
		slk::SLK water_slk("TerrainArt/Water.slk", flags);

		// load the names for terrain textures and cliffs
		cliff_slk.substitute(world_edit_strings, "WorldEditStrings");
		terrain_slk.substitute(world_edit_strings, "WorldEditStrings");

		// load cliff types and terrain texture from the .slk files
		// we assume that the .slk files contain no errors since they are loaded from the casc
		load_cliffs(cliff_slk);
		load_terrain_tex(terrain_slk);

		// link terrain textures and cliff types
		for (const auto& [cliff_id, cliff] : cliff_type_map) {
			if (auto it = terrain_texture_map.find(cliff.ground_tile); it != terrain_texture_map.end()) {
				it->second.cliff_type_id = cliff_id;
			}
		}

		// load pathing overrides from the json
		load_pathing_overrides();

		// load tilesets from world edit data
		// should be used after loading terrain textures
		load_tilesets(water_slk);
	}

	void save() const {
		nlohmann::json root = nlohmann::json::array();

		// create the json object for custom pathing
		for (const auto& [id, texture] : terrain_texture_map) {
			if (!texture.override_pathing.has_value()) {
				continue;
			}
			root.push_back({{"id", id}, {"pathing", texture.override_pathing.value()}});
		}

		// dump, also create parent directory if it doesnt exist
		const auto pathing_file = paths::terrain_pathing_file(hierarchy.map_directory);
		std::filesystem::create_directories(pathing_file.parent_path());
		std::ofstream file(pathing_file);
		if (file) {
			file << root.dump(1, '\t') << '\n';
		} else {
			QMessageBox::critical(nullptr, "Error saving custom pathing", QString("Failed to save %1").arg(pathing_file.string().c_str()));
		}
	}

  private:
	// All available terrain textures and cliff types in the game
	hive::unordered_map<std::string, TerrainTexture> terrain_texture_map;
	hive::unordered_map<std::string, CliffType> cliff_type_map;
	hive::unordered_map<char, Tileset> tileset_map;

	void load_cliffs(slk::SLK& cliff_slk) {
		for (const auto& [cliff_id, _] : cliff_slk.row_headers) {
			CliffType cliff;
			cliff.id = cliff_id;
			cliff.name = cliff_slk.data("name", cliff_id);
			cliff.cliff_model_dir = cliff_slk.data("cliffmodeldir", cliff_id);
			cliff.ramp_model_dir = cliff_slk.data("rampmodeldir", cliff_id);
			cliff.file_path = std::format("{}/{}.dds", cliff_slk.data("texdir", cliff_id), cliff_slk.data("texfile", cliff_id));
			cliff.ground_tile = cliff_slk.data("groundtile", cliff_id);
			cliff_type_map.emplace(cliff_id, std::move(cliff));
		}
	}

	void load_terrain_tex(slk::SLK& terrain_slk) {
		for (const auto& [tile_id, index] : terrain_slk.row_headers) {
			TerrainTexture texture;
			texture.id = tile_id;
			texture.name = terrain_slk.data("name", tile_id);
			texture.file_path = std::format("{}/{}.dds", terrain_slk.data("dir", tile_id), terrain_slk.data("file", tile_id));
			texture.base_pathing = (!terrain_slk.data<bool>("buildable", tile_id) ? PathingMap::Flags::unbuildable : 0)
				| (!terrain_slk.data<bool>("walkable", tile_id) ? PathingMap::Flags::unwalkable : 0)
				| (!terrain_slk.data<bool>("flyable", tile_id) ? PathingMap::Flags::unflyable : 0);
			terrain_texture_map.emplace(tile_id, std::move(texture));
		}
	}

	void load_pathing_overrides() {
		if (std::ifstream file(paths::terrain_pathing_file(hierarchy.map_directory)); file.is_open()) {
			try {
				const nlohmann::json root = nlohmann::json::parse(file);
				for (const auto& entry : root) {
					if (auto it = terrain_texture_map.find(entry["id"].get<std::string>()); it != terrain_texture_map.end()) {
						it->second.override_pathing = entry["pathing"].get<uint8_t>();
					}
				}
			} catch (const nlohmann::json::exception& e) {
				// throw and error message if the json is corrupted or failed to load for some reason
				QMessageBox::critical(
					nullptr,
					"Error loading terrain pathing",
					QString("Failed to load %1:\n%2")
						.arg(paths::terrain_pathing_file(hierarchy.map_directory).string().c_str())
						.arg(e.what())
				);
			}
		}
	}

	void load_tilesets(slk::SLK& water_slk) {
		// load tilesets and water
		for (const auto& [key, value] : world_edit_data.section("TileSets")) {
			if (key.empty() || value.empty()) {
				continue;
			}
			Tileset tileset;
			tileset.id = key.front();
			tileset.name = value[0];
			if (value.size() > 1) {
				tileset.blight_texture = value[1] + ".dds";
			}

			// water data key is e.g. "LSha"
			std::string water_key = key + "Sha";

			tileset.water_id = water_key;
			tileset.water_texture = water_slk.data<std::string_view>("texfile", water_key);

			tileset.water_offset = water_slk.data<float>("height", water_key);
			tileset.water_textures_nr = water_slk.data<int>("numtex", water_key);
			tileset.water_animation_rate = water_slk.data<int>("texrate", water_key);

			int red = water_slk.data<int>("smin_r", water_key);
			int green = water_slk.data<int>("smin_g", water_key);
			int blue = water_slk.data<int>("smin_b", water_key);
			int alpha = water_slk.data<int>("smin_a", water_key);
			tileset.shallow_color_min = glm::vec4(red, green, blue, alpha) / 255.f;

			red = water_slk.data<int>("smax_r", water_key);
			green = water_slk.data<int>("smax_g", water_key);
			blue = water_slk.data<int>("smax_b", water_key);
			alpha = water_slk.data<int>("smax_a", water_key);
			tileset.shallow_color_max = glm::vec4(red, green, blue, alpha) / 255.f;

			red = water_slk.data<int>("dmin_r", water_key);
			green = water_slk.data<int>("dmin_g", water_key);
			blue = water_slk.data<int>("dmin_b", water_key);
			alpha = water_slk.data<int>("dmin_a", water_key);
			tileset.deep_color_min = glm::vec4(red, green, blue, alpha) / 255.f;

			red = water_slk.data<int>("dmax_r", water_key);
			green = water_slk.data<int>("dmax_g", water_key);
			blue = water_slk.data<int>("dmax_b", water_key);
			alpha = water_slk.data<int>("dmax_a", water_key);
			tileset.deep_color_max = glm::vec4(red, green, blue, alpha) / 255.f;

			tileset_map.emplace(tileset.id, std::move(tileset));
		}

		// load sound and DNC paths
		auto load_string = [&](const auto& section_name, auto&& setter) {
			for (const auto& [key, value] : world_edit_data.section(section_name)) {
				if (key.empty() || value.empty()) {
					continue;
				}

				const auto& id = key.front();
				const auto& val = value[0];

				if (auto it = tileset_map.find(id); it != tileset_map.end()) {
					setter(it->second, val);
				}
			}
		};

		load_string("DayAmbience", [](auto& tileset, const auto& val) {
			tileset.day_ambience_sound = val;
		});

		load_string("NightAmbience", [](auto& tileset, const auto& val) {
			tileset.night_ambience_sound = val;
		});

		load_string("SoundEnvironment", [](auto& tileset, const auto& val) {
			tileset.sound_environment = val;
		});

		load_string("TerrainLights", [](auto& tileset, const auto& val) {
			tileset.terrain_dnc = val;
		});

		load_string("UnitLights", [](auto& tileset, const auto& val) {
			tileset.unit_dnc = val;
		});

		// link terrain textures to their tileset
		for (const auto& [tile_id, texture] : terrain_texture_map) {
			const char tileset_key = tile_id.front();
			if (auto it = tileset_map.find(tileset_key); it != tileset_map.end()) {
				it->second.terrain_textures.push_back(tile_id);
			}
		}
	}
};
