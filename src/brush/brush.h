#pragma once

#include <QMouseEvent>
#include <QKeyEvent>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string_view>

import Shader;

class Brush : public QObject {
	Q_OBJECT

public:
	enum class Shape {
		square,
		circle,
		diamond
	};

	enum class Mode {
		placement,
		selection,
		pasting
	};

	bool uv_offset_locked = false;
	glm::ivec2 uv_offset = { 0, 0 };
	int size_granularity = 1;
	int uv_offset_granularity = 4;
	float granularity = 1.f;

	glm::vec2 brush_offset = { 0, 0 };

	glm::u8vec4 brush_color = { 0, 255, 0, 128 };

	GLuint brush_texture;

	glm::vec2 position_new;

	Brush();

	virtual void set_position(const glm::vec2& position);
	virtual glm::vec2 get_position() const;
	virtual void set_size(int size);
	virtual void set_shape(Shape shape);
	virtual void increase_size(int size);
	virtual void decrease_size(int size);
	virtual bool contains(int x, int y) const;

	virtual void switch_mode();
	Mode get_mode() {
		return mode;
	}

	virtual void key_press_event(QKeyEvent* event);
	virtual void key_release_event(QKeyEvent* event) {}
	virtual void mouse_move_event(QMouseEvent* event, double frame_delta);
	virtual void mouse_press_event(QMouseEvent* event, double frame_delta);
	virtual void mouse_release_event(QMouseEvent* event);

	virtual void delete_selection() {};
	virtual void copy_selection() {};
	virtual void cut_selection() {};
	virtual void clear_selection() {};
	virtual void place_clipboard() {};

	virtual void clear_clipboard() {};
	
	void render();
	virtual void render_selector() const;
	virtual void render_selection() const {};
	virtual void render_clipboard() {}
	virtual void render_brush();

	virtual bool can_place() {
		return true;
	};
	virtual void apply_begin() {};
	virtual void apply(double frame_delta) = 0;
	virtual void apply_end() {};

protected:
	Shape shape = Shape::circle;
	Mode mode = Mode::placement;
	Mode return_mode = Mode::placement;

	int size = 1;
	glm::ivec2 position;

	bool selection_started = false;
	glm::vec2 selection_start;

	std::shared_ptr<Shader> selection_shader;
	std::shared_ptr<Shader> selection_circle_shader;
	std::shared_ptr<Shader> brush_shader;

public slots:
	virtual void unselect_id(std::string_view id) {
	}

signals:
	void selection_changed();
};