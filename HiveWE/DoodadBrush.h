#pragma once

class DoodadBrush : public Brush {
public:
	std::string id;
	int variation;
	std::shared_ptr<StaticMesh> mesh;

	bool free_placement;
	bool free_rotation;

	void key_press_event(QKeyEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;

	std::vector<Doodad*> selections;

	void apply() override;
	void render_brush() const override;
	void render_selectionn() const override;

	void set_doodad(const std::string& id);
};