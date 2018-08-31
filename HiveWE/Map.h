#pragma once

class Map {
public:
	bool loaded = false;

	TriggerStrings trigger_strings;
	Triggers triggers;
	MapInfo info;
	Terrain terrain;
	PathingMap pathing_map;
	Doodads doodads;
	Units units;
	Imports imports;

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

	fs::path filesystem_path;

	// Temporary for instancing. Replace by some kind of scenenode
	std::vector<StaticMesh*> meshes;

	double terrain_time;
	double terrain_tiles_time;
	double terrain_water_time;
	double terrain_cliff_time;
	double mouse_world_time;
	double doodad_time;
	double unit_time;
	double render_time;
	double total_time;
	double total_time_min = std::numeric_limits<double>::max();
	double total_time_max = std::numeric_limits<double>::min();

	bool show_timings = false;

	~Map();

	void load(const fs::path& path);
	bool save(const fs::path& path) const;
	void play_test();
	void render(int width, int height);
};