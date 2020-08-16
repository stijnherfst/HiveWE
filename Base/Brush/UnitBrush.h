#pragma once

#include <set>
#include <vector>

#include "Units.h"
#include "Brush.h"
#include "SkinnedMesh.h"
#include "PathingTexture.h"

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

	std::vector<Unit*> selections;
	glm::vec2 clipboard_mouse_position;
	bool clipboard_free_placement = false;
	std::vector<Unit> clipboard;

	UnitBrush();

	void set_shape(const Shape new_shape) override;

	void key_press_event(QKeyEvent* event) override;
	void key_release_event(QKeyEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;
	void mouse_move_event(QMouseEvent* event) override;

	void delete_selection() override;
	void copy_selection() override;
	void cut_selection() override;
	void clear_selection() override;
	void place_clipboard() override;

	void apply_begin() override;
	void apply() override;
	void apply_end() override;
	void render_brush() override;
	void render_selection() const override;
	void render_clipboard() const override;

	void set_random_rotation();
	void set_unit(const std::string& id);
};