#pragma once

#include <set>
#include <vector>
#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "units.h"
#include "brush.h"


import SkinnedMesh;
import PathingTexture;

class UnitBrush : public Brush {
	std::string id;

	std::shared_ptr<SkinnedMesh> mesh;
	SkeletalModelInstance skeleton;

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

	UnitBrush();

	void set_shape(const Shape new_shape) override;

	void key_press_event(QKeyEvent* event) override;
	void key_release_event(QKeyEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;
	void mouse_press_event(QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(QMouseEvent* event, double frame_delta) override;

	void delete_selection() override;
	void copy_selection() override;
	void cut_selection() override;
	void clear_selection() override;
	void place_clipboard() override;

	void apply_begin() override;
	void apply(double frame_delta) override;
	void apply_end() override;
	void render_brush() override;
	void render_selection() const override;
	void render_clipboard() override;

	void set_random_rotation();
	void set_unit(const std::string& id);

	void unselect_id(std::string_view id) override;
};