#pragma once

class PathingMap {
	static constexpr int write_version = 0;

	public:
	int width;
	int height;

	enum Flags {
		unwalkable	= 0b00000010,
		unflyable	= 0b00000100,
		unbuildable = 0b00001000,
	};

	GLuint texture_static;
	GLuint texture_dynamic;
	std::vector<uint8_t> pathing_cells_static;
	std::vector<uint8_t> pathing_cells_dynamic;
	
	bool load(BinaryReader& reader);
	void save() const;

	void update_dynamic();
};