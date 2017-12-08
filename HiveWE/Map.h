#pragma once

class Map {
public:
	Terrain terrain;
	PathingMap pathing_map;

	void load(std::wstring path);
};