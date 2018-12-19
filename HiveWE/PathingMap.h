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

	// For undo/redo
	std::vector<uint8_t> old_pathing_cells_static;
	
	bool load(BinaryReader& reader);
	void save() const;

	void update_dynamic();
	void upload_static_pathing();


	void new_undo_group();
	void add_undo(const QRect& area);
};

// Undo/redo structures
class PathingMapAction : public TerrainUndoAction {
public:
	QRect area;
	std::vector<uint8_t> old_pathing;
	std::vector<uint8_t> new_pathing;

	void undo() override;
	void redo() override;
};