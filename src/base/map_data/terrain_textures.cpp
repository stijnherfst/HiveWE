module;

#include <QMessageBox>

module MapData;

import std;
import SLK;
import PathingMap;
import Hierarchy;
import <nlohmann/json.hpp>;

void MapData::load_terrain_data() {
	slk::SLK terrain_slk;
	slk::SLK cliff_slk;

	// load terrain and cliff .slk files
	// the game does not let the map change those, so we will load them directly from casc
	terrain_slk.load("TerrainArt/Terrain.slk", false);
	cliff_slk.load("TerrainArt/CliffTypes.slk", false);

	// load cliff types from the .slk file
	// we assume that the .slk files contain no errors since they are loaded from the casc
	for (const auto& [cliff_id, _] : cliff_slk.row_headers) {
		CliffType cliff;
		cliff.id = cliff_id;
		cliff.name = cliff_slk.data("name", cliff_id);
		cliff.cliff_model_dir = cliff_slk.data("cliffmodeldir", cliff_id);
		cliff.ramp_model_dir = cliff_slk.data("rampmodeldir", cliff_id);
		cliff.file_path = std::format("{}/{}", cliff_slk.data("texdir", cliff_id), cliff_slk.data("texfile", cliff_id));
		cliff.ground_tile = cliff_slk.data("groundtile", cliff_id);
		m_cliff_types.emplace(cliff_id, std::move(cliff));
	}

	// load terrain textures from the .slk file
	for (const auto& [tile_id, index] : terrain_slk.row_headers) {
		TerrainTexture texture;
		texture.id = tile_id;
		texture.name = terrain_slk.data("comment", tile_id);
		texture.file_path = std::format("{}/{}", terrain_slk.data("dir", tile_id), terrain_slk.data("file", tile_id));
		texture.base_pathing = (!terrain_slk.data<bool>("buildable", tile_id) ? PathingMap::Flags::unbuildable : 0)
			| (!terrain_slk.data<bool>("walkable", tile_id) ? PathingMap::Flags::unwalkable : 0)
			| (!terrain_slk.data<bool>("flyable", tile_id) ? PathingMap::Flags::unflyable : 0);
		m_terrain_textures.emplace(tile_id, std::move(texture));
	}

	// link terrain textures and cliff types
	for (const auto& [cliff_id, cliff] : m_cliff_types) {
		if (auto it = m_terrain_textures.find(cliff.ground_tile); it != m_terrain_textures.end()) {
			it->second.cliff_type_id = cliff_id;
		}
	}

	// load pathing overrides from the json
	if (std::ifstream file(MapData::terrain_pathing_file()); file.is_open()) {
		try {
			const nlohmann::json root = nlohmann::json::parse(file);
			for (const auto& entry : root) {
				if (auto it = m_terrain_textures.find(entry["id"].get<std::string>()); it != m_terrain_textures.end()) {
					it->second.override_pathing = entry["pathing"].get<uint8_t>();
				}
			}
		} catch (const nlohmann::json::exception& e) {
			// throw and error message if the json is corrupted or failed to load for some reason
			QMessageBox::critical(
				nullptr,
				"Error loading terrain pathing",
				QString("Failed to load %1:\n%2").arg(MapData::terrain_pathing_file().string().c_str()).arg(e.what())
			);
		}
	}
}

void MapData::save_terrain_data() const {
	nlohmann::json root = nlohmann::json::array();

	// create the json object for custom pathing
	for (const auto& [id, texture] : m_terrain_textures) {
		if (!texture.override_pathing.has_value()) {
			continue;
		}
		root.push_back({{"id", id}, {"pathing", texture.override_pathing.value()}});
	}

	// dump, also create parent directory if it doesnt exist
	std::filesystem::create_directories(MapData::hive_data_folder());
	std::ofstream file(MapData::terrain_pathing_file());
	file << root.dump(1, '\t') << '\n';
}
