#pragma once

class Map {
public:
	Terrain terrain;
	PathingMap pathing_map;
	Doodads doodads;

	void load(std::wstring path);
};