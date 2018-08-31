#pragma once

class Brush {
public:
	enum class Shape {
		square,
		circle,
		diamond
	};

	enum class Mode {
		placement,
		selection
	};

	int granularity = 1;
	bool uv_offset_locked = false;
	glm::ivec2 uv_offset;
	glm::vec2 brush_offset = { 0, 0 };

	void create();
	virtual void set_position(const glm::vec2& position);
	glm::vec2 get_position() const;
	void set_size(int size);
	void set_shape(Shape shape);
	void increase_size(int size);
	void decrease_size(int size);
	bool contains(int x, int y) const;

	virtual void key_press_event(QKeyEvent* event);
	virtual void mouse_move_event(QMouseEvent* event);
	virtual void mouse_press_event(QMouseEvent* event);
	virtual void mouse_release_event(QMouseEvent* event);

	void render() const;
	virtual void render_selection() const;
	virtual void render_brush() const;
	virtual void render_selectionn() const {};

	virtual void apply() = 0;
	virtual void apply_end() {};
protected:
	Shape shape = Shape::circle;
	Mode mode = Mode::placement;

	int size = 0;
	glm::ivec2 position;

	bool selection_started = false;
	glm::vec2 selection_start;

	std::vector<glm::u8vec4> brush;
	GLuint brush_texture;

	std::shared_ptr<Shader> selection_shader;
	std::shared_ptr<Shader> brush_shader;
};