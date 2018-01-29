#pragma once

class PathingMap {
	public:
	int width;
	int height;

	GLuint pathing_texture;
	std::vector<uint8_t> pathing_cells;
	
	bool load(BinaryReader& reader, Terrain& terrain);
};