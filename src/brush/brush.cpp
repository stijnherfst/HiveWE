#include "brush.h"

import std;
import Camera;
import OpenGLUtilities;
import ResourceManager;
import Globals;
import <glad/glad.h>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

Brush::Brush() {
	set_size(size);

	selection_shader = resource_manager.load<Shader>({"data/shaders/selection.vert", "data/shaders/selection.frag"});
	selection_circle_shader = resource_manager.load<Shader>({"data/shaders/selection_circle.vert", "data/shaders/selection_circle.frag"});
	brush_shader = resource_manager.load<Shader>({"data/shaders/brush.vert", "data/shaders/brush.frag"});
}

glm::vec2 Brush::get_position() const {
	const bool even_width = size.x % (2 * static_cast<size_t>(size_granularity)) == 0;
	const bool even_height = size.y % (2 * static_cast<size_t>(size_granularity)) == 0;

	glm::vec2 even = glm::floor(glm::vec2(input_handler.mouse_world) * position_granularity + 0.5f) / position_granularity;
	glm::vec2 uneven = glm::floor(glm::vec2(input_handler.mouse_world) * position_granularity) / position_granularity + (1.f / position_granularity / 2.f);

	if (center_on_tile_corner) {
		std::swap(even, uneven);
	}

	glm::vec2 result;
	if (even_width) {
		result.x = even.x;
	} else {
		result.x = uneven.x;
	}

	if (even_height) {
		result.y = even.y;
	} else {
		result.y = uneven.y;
	}

	return result;
}

void Brush::set_size(const glm::ivec2 new_size) {
	size = glm::clamp(new_size * size_granularity, 1, 999);
	set_shape(shape);
}

void Brush::set_shape(const Shape new_shape) {
	context->makeCurrent();
	std::vector<glm::u8vec4> brush(size.x * size.y, {0, 0, 0, 0});

	shape = new_shape;

	for (int i = 0; i < size.x; i++) {
		for (int j = 0; j < size.y; j++) {
			if (contains(glm::ivec2(i, j) / size_granularity)) {
				brush[j * size.x + i] = brush_color;
			} else {
				brush[j * size.x + i] = {0, 0, 0, 0};
			}
		}
	}

	glDeleteTextures(1, &brush_texture);
	glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTextureStorage2D(brush_texture, 1, GL_RGBA8, size.x, size.y);
	glTextureSubImage2D(brush_texture, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, brush.data());
}

/// Whether the brush shape contains the point, Arguments in brush coordinates
bool Brush::contains(const glm::ivec2 pos) const {
	switch (shape) {
		case Shape::square:
			return true;
		case Shape::circle: {
			const int half_size = (size.x / 2) / size_granularity;
			const int distance = (pos.x - half_size) * (pos.x - half_size) + (pos.y - half_size) * (pos.y - half_size);
			return distance <= half_size * half_size;
		}
		case Shape::diamond:
			const int half_size = (size.x / 2) / size_granularity;
			return std::abs(pos.x - half_size) + std::abs(pos.y - half_size) <= half_size;
	}
	return true;
}

void Brush::increase_size(const int new_size) {
	set_size(size / size_granularity + new_size);
}

void Brush::decrease_size(const int new_size) {
	set_size(size / size_granularity - new_size);
}

void Brush::switch_mode() {
	if (mode != Mode::placement && mode != Mode::selection) {
		mode = return_mode;
	}
	mode = (mode == Mode::placement) ? Mode::selection : Mode::placement;

	clear_selection();
	selection_started = false;
}

void Brush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Escape:
			clear_selection();
			break;
		case Qt::Key_Equal:
			increase_size(2);
			break;
		case Qt::Key_Minus:
			decrease_size(2);
			break;
		case Qt::Key_Delete:
			delete_selection();
			break;
		case Qt::Key_X:
			if (event->modifiers() & Qt::ControlModifier) {
				cut_selection();
			}
			break;
		case Qt::Key_C:
			if (event->modifiers() & Qt::ControlModifier) {
				copy_selection();
			}
			break;
		case Qt::Key_V:
			if (event->modifiers() & Qt::ControlModifier) {
				return_mode = mode;
				mode = Mode::pasting;
			}
			break;
	}
}

void Brush::mouse_move_event(QMouseEvent* event, double frame_delta) {
	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::placement && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
			apply(frame_delta);
		}
	}
}

void Brush::mouse_press_event(QMouseEvent* event, double frame_delta) {
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (!event->modifiers()) {
		clear_selection();
	}

	if (mode == Mode::selection && !(event->modifiers() & Qt::ControlModifier)) {
		if (!selection_started) {
			selection_started = true;
			selection_start = input_handler.mouse_world;
		}
	} else if (mode == Mode::placement) {
		// Check if eligible for placement
		if (event->button() == Qt::LeftButton) {
			apply_begin();
			if (can_place() || event->modifiers() & Qt::ShiftModifier) {
				apply(0.5);
			}
		}
	} else if (mode == Mode::pasting && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
		clear_selection();
		place_clipboard();
		mode = Mode::selection;
	}
}

void Brush::mouse_release_event(QMouseEvent* event) {
	if (mode == Mode::selection) {
		selection_started = false;
	} else if (mode == Mode::placement) {
		if (event->button() == Qt::LeftButton) {
			apply_end();
		}
	}
}

void Brush::render() {
	if (mode == Mode::selection) {
		render_selector();
	}
	if (mode == Mode::placement) {
		render_brush();
	}
	if (mode == Mode::pasting) {
		render_clipboard();
	}
	render_selection();
}

void Brush::render_selector() const {
	if (selection_started) {
		glDisable(GL_DEPTH_TEST);

		selection_shader->use();

		glm::mat4 model(1.f);
		model = glm::translate(model, selection_start);
		model = glm::scale(model, glm::vec3(glm::vec2(input_handler.mouse_world), 1.f) - glm::vec3(glm::vec2(selection_start), 1.f));
		model = camera.projection_view * model;
		glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glDrawArrays(GL_LINE_LOOP, 0, 4);

		glDisableVertexAttribArray(0);
		glEnable(GL_DEPTH_TEST);
	}
}

void Brush::render_brush() {}
