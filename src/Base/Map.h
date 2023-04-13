#pragma once

#include "Triggers.h"
//#include "MapInfo.h"
#include "Terrain.h"
#include "Doodads.h"

#include "Brush.h"
#include "Units.h"
#include "RenderManager.h"
#include <filesystem> // Filesystem include at the bottom due to QTBUG-73263
#include <map>

import GameCameras;
import Imports;
import MapInfo;
import Sounds;
import Regions;
import TerrainUndo;
import TriggerStrings;
import PathingMap;
import Physics;

namespace fs = std::filesystem;

class Map : public QObject {
	Q_OBJECT

  public:
	bool loaded = false;

	TriggerStrings trigger_strings;
	Triggers triggers;
	MapInfo info;
	Terrain terrain;
	TerrainUndo terrain_undo;
	PathingMap pathing_map;
	Imports imports;
	Doodads doodads;
	Units units;
	Regions regions;
	GameCameras cameras;
	Sounds sounds;
	//ShadowMap shadow_map;

	Brush* brush = nullptr;
	Physics physics;

	bool enforce_water_height_limits = true;

	bool render_doodads = true;
	bool render_units = true;
	bool render_pathing = false;
	bool render_brush = true;
	bool render_lighting = true;
	bool render_wireframe = false;
	bool render_debug = false;

	glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 1.f, -3.f));

	fs::path filesystem_path;
	std::string name;

	RenderManager render_manager;

	void load(const fs::path& path);
	bool save(const fs::path& path);

	void update(double delta, int width, int height);
	void render();

	void resize(size_t width, size_t height);

	std::string get_unique_id(bool first_uppercase);
};