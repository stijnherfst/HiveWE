#pragma once

class DoodadBrush : public Brush {
	std::set<int> possible_variations = { 0 };
	int get_random_variation();

	std::string id;
	int variation = 0;

	std::shared_ptr<StaticMesh> mesh;
public:
	Doodad::State state = Doodad::State::visible_solid;
	std::shared_ptr<Texture> pathing_texture;

	bool free_placement = false;
	bool free_rotation = false;

	bool random_variation = true;
	bool random_scale = true;
	bool random_rotation = true;

	float scale = 1.f;
	float min_scale = 1.f;
	float max_scale = 1.f;

	float rotation = 0.f;

	std::vector<Doodad*> selections;
	glm::vec2 clipboard_mouse_position;
	bool clipboard_free_placement = false;
	std::vector<Doodad> clipboard;

	std::unique_ptr<DoodadAddAction> doodad_undo;

	DoodadBrush();

	void set_shape(const Shape new_shape) override;

	void key_press_event(QKeyEvent* event) override;
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
	void render_brush() const override;
	void render_selection() const override;
	void render_clipboard() const override;

	void set_random_variation();
	void add_variation(int variation);
	void erase_variation(int variation);

	void set_doodad(const std::string& id);
};