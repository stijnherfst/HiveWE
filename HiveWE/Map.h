#pragma once

class Map {
public:
	Terrain terrain;
	PathingMap pathing_map;
	Doodads doodads;

	PathingBrush brush;

	bool render_doodads = true;
	bool render_brush = true;
	bool render_pathing = true;

	fs::path filesystem_path;

	void load(fs::path path);
	void close();

	bool save(fs::path path);

	void play_test();

	void render();
};