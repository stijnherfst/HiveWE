#pragma once

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

	bool is_protected = false;
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

	// For instancing
	std::vector<StaticMesh*> meshes;

	std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
	double total_time;

	void load(const fs::path& path);
	bool save(const fs::path& path);
	void render(int width, int height);
};