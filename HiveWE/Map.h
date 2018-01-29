#pragma once

class Map {
public:
	Terrain terrain;
	PathingMap pathing_map;
	Doodads doodads;

	Brush* brush;

	bool render_doodads = true;
	bool render_brush = true;

	void load(std::wstring path);
	void render();
};