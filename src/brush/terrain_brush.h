#pragma once

#include <string>
#include <map>
#include <vector>
#include <array>
#include <functional>

#include "brush.h"
#include "terrain_operators.h"

import Doodad;
import Terrain;
import TerrainUndo;
import WorldUndoManager;

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

	void apply_begin() override;
	void apply(double frame_delta) override;
	void apply_end() override;

	void add_terrain_undo(WorldEditContext& ctx, const QRect& area, TerrainUndoType type);
	void add_pathing_undo(WorldEditContext& ctx, const QRect& area);

	// all terrain operators
	CliffOperator cliff_operator;
	HeightOperator height_operator;
	TextureOperator texture_operator;
	CellOperator cell_operator;

	/// Deactivates the specified operator
	void deactivate_operator(TerrainOperator& target);

	/// Activates the target terrain operator. Also disables all
	/// active operators which cannot be used simultaneously
	void activate_operator(TerrainOperator& target);

	/// Returns true if two terrain operators are allowed to be active simultaneously
	bool can_combine(const TerrainOperator& a, const TerrainOperator& b) const;

	/// Returns the unclipped top-left corner of the brush area in terrain coordinates
	glm::ivec2 get_unclipped_pos() const;

	/// Converts a rect in pathing resoltion to a rect in terrain resolution
	static QRect from_pathing_rect(const QRect& rect);

	/// Converts a rect in terrain resolution to a rect in pathing resolution
	static QRect to_pathing_rect(const QRect& rect);

  private:
	/// Area which was modified in the last operation in pathing map resolution
	QRect updated_area;

	// undo/redo stuff
	std::vector<Doodad> pre_change_doodads;
	std::map<int, Doodad> post_change_doodads;

	std::vector<Corner> old_corners;
	int old_corners_width = 0;
	int old_corners_height = 0;
	std::vector<uint8_t> old_pathing_cells_static;

	std::array<std::reference_wrapper<TerrainOperator>, 4> terrain_operators;

	// checks if there is an active operator
	bool has_active_operators();
};
