#pragma once

class DoodadBrush : public Brush {
	std::set<int> possible_variations = { 0 };
	int get_random_variation();

	std::string id;
	int variation;
	std::shared_ptr<StaticMesh> mesh;
	std::shared_ptr<Texture> pathing_texture;
public:

	bool free_placement;
	bool free_rotation;

	bool random_variation = true;

	void key_press_event(QKeyEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;
	void mouse_move_event(QMouseEvent* event) override;

	std::vector<Doodad*> selections;

	void apply() override;
	void render_brush() const override;
	void render_selection() const override;

	void set_random_variation();
	void add_variation(int variation);
	void erase_variation(int variation);

	void set_doodad(const std::string& id);
};