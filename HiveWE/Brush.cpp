#include "stdafx.h"

void Brush::create() {
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	set_size(size);

	selection_shader = resource_manager.load<Shader>({ "Data/Shaders/selection.vs", "Data/Shaders/selection.fs" });
	brush_shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2& new_position) {
	const glm::vec2 center_position = new_position - size / 2.f * 0.25f + brush_offset;

	position = glm::floor(center_position);
	if (!uv_offset_locked) {
		glm::vec2 decimals = center_position - glm::vec2(position);

		switch (uv_offset_granularity) {
			case 1:
				uv_offset = { 0.f, 0.f };
				break;
			case 2:
				decimals *= 2.f;
				decimals = glm::floor(decimals);
				decimals *= 2.f;
				uv_offset = glm::abs(decimals);
				break;
			case 4:
				uv_offset = glm::abs(decimals * 4.f);
				break;
		}
	}
}

glm::vec2 Brush::get_position() const {
	return glm::vec2(position);
}

void Brush::set_size(const int new_size) {
	//const int change = new_size * size_granularity - size;

	size = std::clamp(new_size * size_granularity, 1, 240);

	brush.clear();
	brush.resize(size * size, { 0, 0, 0, 0 });

	set_shape(shape);

	set_position(glm::vec2(position) + glm::vec2(uv_offset));
}

void Brush::set_shape(const Shape new_shape) {
	shape = new_shape;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (contains(i / size_granularity, j / size_granularity)) {
				brush[j * size + i] = brush_color;
			} else {
				brush[j * size + i] = { 0, 0, 0, 0 };
			}
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

/// Whether the brush shape contains the point, Arguments in brush coordinates
bool Brush::contains(const int x, const int y) const {
	switch (shape) {
		case Shape::square:
			return true;
		case Shape::circle: {
			int half_size = (size / 2) / size_granularity;
			int distance = (x - half_size) * (x - half_size);
			distance += (y - half_size) * (y - half_size);
			return distance <= half_size * half_size;
		}
		case Shape::diamond:
			int half_size = (size / 2) / size_granularity;

			return std::abs(x - half_size) + std::abs(y - half_size) <= half_size;
	}
	return true;
}

void Brush::increase_size(const int new_size) {
	set_size(size + new_size);
}
void Brush::decrease_size(const int new_size) {
	set_size(size - new_size);
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
		case Qt::Key_Equal:
			increase_size(1);
			break;
		case Qt::Key_Minus:
			decrease_size(1);
			break;
		case Qt::Key_Delete:
			delete_selection();
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

void Brush::mouse_move_event(QMouseEvent* event) {
	set_position(input_handler.mouse_world);

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
		} else if (mode == Mode::placement) {
			apply();
		}
	}
}

void Brush::mouse_press_event(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (mode == Mode::selection && !(event->modifiers() & Qt::ControlModifier)) {
		if (!selection_started) {
			selection_started = true;
			selection_start = input_handler.mouse_world;
		}
	} else if (mode == Mode::placement) {
		if (event->button() == Qt::LeftButton) {
			apply_begin();
			apply();
		}
	} else if (mode == Mode::pasting) {
		clear_selection();
		place_clipboard();
		mode == Mode::selection;
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

void Brush::render() const {
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
		gl->glDisable(GL_DEPTH_TEST);

		selection_shader->use();

		glm::mat4 model(1.f);
		model = glm::translate(model, glm::vec3(selection_start, 0.f));
		model = glm::scale(model, glm::vec3(glm::vec2(input_handler.mouse_world), 1.f) - glm::vec3(selection_start, 1.f));
		model = camera->projection_view * model;
		gl->glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		gl->glEnableVertexAttribArray(0);
		gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		gl->glDrawArrays(GL_LINE_LOOP, 0, 4);

		gl->glDisableVertexAttribArray(0);
		gl->glEnable(GL_DEPTH_TEST);
	}
}

void Brush::render_brush() const {
	gl->glDisable(GL_DEPTH_TEST);

	brush_shader->use();

	const int cells = std::ceil(size / 4.f) + 1;

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera->projection_view[0][0]);
	gl->glUniform2f(2, position.x, position.y);
	gl->glUniform2f(3, uv_offset.x, uv_offset.y);
	gl->glUniform1i(4, cells);

	gl->glBindTextureUnit(0, map->terrain.ground_corner_height);
	gl->glBindTextureUnit(1, brush_texture);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, cells * cells);

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}