#pragma once

#include <QMouseEvent>
#include <QKeyEvent>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
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


	GLuint brush_texture;

	Brush();

	virtual glm::vec2 get_position() const;
	virtual void set_size(glm::ivec2 size);
	virtual void set_shape(Shape shape);
	virtual void increase_size(int size);
	virtual void decrease_size(int size);
	virtual bool contains(glm::ivec2 pos) const;

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

	/// The color used to render the brush which currently mimics the WC3 default
	glm::u8vec4 brush_color = { 0, 255, 0, 128 };

	/// How many quarter tiles fit into one size unit for this brush. Terrain brush is 4, pathing brush is 1.
	int size_granularity = 1;
	/// The granularity of the grid that the position will be snapped to. There will be 1 / position_granularity cells in a tile
	float position_granularity = 1.f;
	/// Whether the brush will be offset to center on a corner of the grid lattice rather than on a grid tile
	bool center_on_tile_corner = false;
	/// Size in 1/4ths of a tile (corresponding to the pathing map tile size which is the smallest)
	glm::ivec2 size = glm::ivec2(1);

	bool selection_started = false;
	glm::vec3 selection_start;

	std::shared_ptr<Shader> selection_shader;
	std::shared_ptr<Shader> selection_circle_shader;
	std::shared_ptr<Shader> brush_shader;

public slots:
	virtual void unselect_id(std::string_view id) {
	}

signals:
	void selection_changed();
};