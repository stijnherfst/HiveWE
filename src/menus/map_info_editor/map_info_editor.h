#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ui_map_info_editor.h"
#include <filesystem>

import Texture;
import MapInfo;
import TriggerStrings;
import Tileset;
import Terrain;
import Map;

class MapInfoEditor: public QDialog {
	Q_OBJECT

  public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	void save() const;

  private:
	void setup_description(const MapInfo& info, const TriggerStrings& trigger_strings);
	void save_description(MapInfo& info, TriggerStrings& trigger_strings) const;

	void setup_loading_screen(const MapInfo& info, const TriggerStrings& trigger_strings, const std::filesystem::path& filesystem_path);
	void save_loading_screen(MapInfo& info, TriggerStrings& trigger_strings) const;

	void setup_options(const MapInfo& info, const TilesetData& tilesets);
	void save_options(MapInfo& info) const;

	void setup_map_size(const Terrain& terrain, const MapInfo& info);
	void save_map_size(Map& map) const;
	void update_map_size_gui();
	void adjust_bounds(int delta_left, int delta_right, int delta_top, int delta_bottom);
	void update_bounds_preview() const;
	void update_bounds_text() const;

	// used for changing map size
	glm::ivec2 old_map_bottom_left;
	glm::ivec2 old_map_top_right;
	glm::ivec2 new_map_bottom_left;
	glm::ivec2 new_map_top_right;

	glm::ivec2 old_playable_bottom_left;
	glm::ivec2 old_playable_top_right;
	glm::ivec2 new_playable_bottom_left;
	glm::ivec2 new_playable_top_right;

	glm::vec2 terrain_offset;

	Texture original_minimap;
};
