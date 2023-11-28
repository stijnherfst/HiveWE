#pragma once

#include <set>
#include <vector>

#include "doodads.h"
#include "brush.h"
import PathingTexture;

class DoodadBrush : public Brush {
	Q_OBJECT

	std::set<int> possible_variations = { 0 };
	int get_random_variation();

	int variation = 0;

	Doodad doodad;
	std::shared_ptr<SkinnedMesh> click_helper;
	SkeletalModelInstance click_helper_skeleton;

  public:
	Doodad::State state = Doodad::State::visible_solid;

	bool random_variation = true;
	bool random_scale = true;
	bool random_rotation = true;

	bool select_doodads = true;
	bool select_destructibles = true;

	bool lock_doodad_z = false;

	std::unique_ptr<DoodadAddAction> doodad_undo;
	std::unique_ptr<DoodadStateAction> doodad_state_undo;

	std::unordered_set<Doodad*> selections;

	glm::vec2 clipboard_mouse_offset;
	std::vector<Doodad> clipboard;
	bool clipboard_free_placement = false;

	bool dragging = false;
	bool dragged = false;
	glm::vec3 drag_start;
	std::vector<glm::vec2> drag_offsets;

	enum class Action {
		none,
		drag,
		move,
		rotate,
		scale
	};

	Action action = Action::none;

	DoodadBrush();

	void set_shape(const Shape new_shape) override;

	void key_press_event(QKeyEvent* event) override;
	void key_release_event(QKeyEvent* event) override;
	void mouse_press_event(QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(QMouseEvent* event, double frame_delta) override;
	void mouse_release_event(QMouseEvent* event) override;

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

	bool can_place() override;

	void set_random_rotation();
	void set_random_variation();
	void add_variation(int variation);
	void erase_variation(int variation);

	void set_doodad(const std::string& id);

	void start_action(Action new_action);
	void end_action();

public slots:
	// Angle in radians
	void set_selection_angle(float angle);
	void set_selection_absolute_height(float height);
	void set_selection_relative_height(float height);
	void set_selection_scale_component(int component, float scale);

	void unselect_id(std::string_view id) override;

signals:
	void angle_changed();
	void scale_changed();
	void position_changed();
	void request_doodad_select(std::string id);
};