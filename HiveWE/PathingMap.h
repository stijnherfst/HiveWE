#pragma once

class PathingMap {
	public:
	size_t width;
	size_t height;

	GLuint pathing_texture;
	std::vector<uint8_t> pathing_cells;
	
	bool load(BinaryReader& reader, Terrain& terrain);
	void save() const;
};