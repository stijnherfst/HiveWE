#pragma once

class DoodadBrush : public Brush {
	std::set<int> possible_variations = { 0 };
	int get_random_variation();

	std::string id;
	int variation;

	std::shared_ptr<StaticMesh> mesh;
public:
	Doodad::State state = Doodad::State::visible_solid;
	std::shared_ptr<Texture> pathing_texture;

	bool free_placement;
	bool free_rotation;

	bool random_variation = true;
	bool random_scale = true;
	bool random_rotation = true;

	float scale = 1.f;
	float min_scale = 1.f;
	float max_scale = 1.f;

	float rotation = 0.f;

	std::vector<Doodad*> selections;

	DoodadBrush();

	void set_shape(const Shape new_shape) override;

	void key_press_event(QKeyEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;
	void mouse_move_event(QMouseEvent* event) override;

	void clear_selection() override;

	void apply() override;
	void render_brush() const override;
	void render_selection() const override;

	void set_random_variation();
	void add_variation(int variation);
	void erase_variation(int variation);

	void set_doodad(const std::string& id);
};