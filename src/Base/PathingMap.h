#pragma once

import BinaryReader;
import PathingTexture;

#include <memory>

#include <QOpenGLFunctions_4_5_Core>
#include <QRect>

#include "TerrainUndo.h"

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

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
	
	bool load();
	void save() const;

	void dynamic_clear_area(const QRect& area);

	bool is_area_free(glm::vec2 position, int rotation, const std::shared_ptr<PathingTexture>& pathing_texture, uint8_t mask);
	void blit_pathing_texture(glm::vec2 position, int rotation, const std::shared_ptr<PathingTexture>& pathing_texture);

	void upload_static_pathing();
	void upload_dynamic_pathing();

	void new_undo_group();
	void add_undo(const QRect& area);

	void resize(size_t width, size_t height);
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