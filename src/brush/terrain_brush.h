#pragma once

#include <string>
#include <map>
#include <vector>

#include "brush.h"

import Doodad;
import Terrain;
import TerrainUndo;

// Forward declarations
class TerrainOperator;
class HeightOperator;
class TextureOperator;
class CliffOperator;
class CellOperator;

class TerrainBrush: public Brush {
	// Friend declarations for terrain operators
	friend class TerrainOperator;
	friend class HeightOperator;
	friend class TextureOperator;
	friend class CliffOperator;
	friend class CellOperator;

  public:
	bool apply_tile_pathing = true;
	bool apply_cliff_pathing = true;
	bool apply_water_pathing = true;
	bool deform_water = false;
	bool deform_ground = true;

	bool enforce_water_height_limits = true;
	bool change_doodad_heights = true;
	bool relative_cliff_heights = false;

	bool dragging = false;
	bool dragged = false;

	TerrainBrush();

	void mouse_release_event(QMouseEvent* event) override;
	void mouse_press_event(QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(QMouseEvent* event, double frame_delta) override;

	void check_nearby(int begx, int begy, int i, int j, QRect& area) const;

	void apply_begin() override;
	void apply(double frame_delta) override;
	void apply_end() override;

	void add_terrain_undo(const QRect& area, TerrainUndoType type);
	void add_pathing_undo(const QRect& area);

	// all terrain operators
	CliffOperator* cliff_operator = nullptr;
	HeightOperator* height_operator = nullptr;
	TextureOperator* texture_operator = nullptr;
	CellOperator* cell_operator = nullptr;

	/// Deactivates the specified operator
	void deactivate_operator(TerrainOperator* target);

	/// Activates the target terrain operator. Also disables all
	/// active operators which cannot be used simultaneously
	void activate_operator(TerrainOperator* target);

  private:
	// undo/redo stuff
	QRect texture_height_area;
	QRect cliff_area;
	QRect pathing_area;

	std::vector<Doodad> pre_change_doodads;
	std::map<int, Doodad> post_change_doodads;

	std::vector<Corner> old_corners;
	int old_corners_width = 0;
	int old_corners_height = 0;
	std::vector<uint8_t> old_pathing_cells_static;

	// terrain operator stuff
	std::vector<TerrainOperator*> terrain_operators;
	void setup_operators();
};
