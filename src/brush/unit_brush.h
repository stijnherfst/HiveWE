#pragma once

#include <set>
#include <vector>
#include <unordered_set>

#include <QKeyEvent>
#include <QMouseEvent>

#include <glm/glm.hpp>
#include "brush.h"

import Units;
import SkinnedMesh;
import PathingTexture;
import UnitsUndo;
import WorldUndoManager;
import RenderManager;
import Terrain;
import PathingMap;
import Skeleton;

class UnitBrush : public Brush {
	std::string id;

	std::shared_ptr<SkinnedMesh> mesh;
	Skeleton skeleton;

  public:
	float rotation = 0.f;
	bool random_rotation = true;

	int player_id = 0;

	std::unique_ptr<UnitAddAction> unit_undo;
	std::unique_ptr<UnitStateAction> unit_state_undo;

	std::unordered_set<Unit*> selections;
	glm::vec2 clipboard_mouse_offset;
	bool clipboard_free_placement = false;
	std::vector<Unit> clipboard;

	bool dragging = false;
	bool dragged = false;
	glm::vec3 drag_start;
	std::vector<glm::vec2> drag_offsets;

	UnitBrush(Units& units, Terrain& terrain, PathingMap& pathing_map,
		  RenderManager& render_manager, WorldUndoManager& world_undo);

	Units& units;
	Terrain& terrain;
	PathingMap& pathing_map;
	RenderManager& render_manager;
	WorldUndoManager& world_undo;

	void set_shape(Shape new_shape) override;

	void key_press_event(WorldEditContext& ctx, const QKeyEvent* event) override;
	void key_release_event(WorldEditContext& ctx, const QKeyEvent* event) override;
	void mouse_release_event(WorldEditContext& ctx, const QMouseEvent* event) override;
	void mouse_press_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta) override;

	void delete_selection() override;
	void copy_selection() override;
	void cut_selection() override;
	void clear_selection() override;
	void place_clipboard(WorldEditContext& ctx) override;

	void apply_begin(WorldEditContext& ctx) override;
	void apply(WorldEditContext& ctx, double frame_delta) override;
	void apply_end(WorldEditContext& ctx) override;
	void render_brush() override;
	void render_selection() const override;
	void render_clipboard() override;

	bool can_place() override;

	void set_random_rotation();
	void set_unit(const std::string& id);

	void unselect_id(std::string_view id) override;
};