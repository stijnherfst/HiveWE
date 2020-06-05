#pragma once

#include <filesystem>

namespace fs = std::filesystem;

#include "TriggerStrings.h"
#include "Triggers.h"
#include "MapInfo.h"
#include "Terrain.h"
#include "TerrainUndo.h"
#include "PathingMap.h"
#include "Imports.h"
#include "Doodads.h"
#include "Units.h"
#include "Regions.h"
#include "GameCameras.h"
#include "Sounds.h"
#include "RenderManager.h"

#include "Brush.h"
#include "StaticMesh.h"
#include "Physics.h"

class Map {
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

	Brush* brush = nullptr;
	Physics physics;

	bool units_loaded = false;

	bool enforce_water_height_limits = true;

	bool render_doodads = true;
	bool render_units = true;
	bool render_pathing = true;
	bool render_brush = true;
	bool render_lighting = true;
	bool render_wireframe = false;
	bool render_debug = false;

	fs::path filesystem_path;
	std::string name;

	RenderManager render_manager;

	std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();
	double total_time;

	void load(const fs::path& path);
	bool save(const fs::path& path);

	void update(double delta, int width, int height);
	void render();
};