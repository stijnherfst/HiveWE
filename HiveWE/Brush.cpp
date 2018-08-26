#include "stdafx.h"

void Brush::create() {
	gl->glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	set_size(size);

	selection_shader = resource_manager.load<Shader>({ "Data/Shaders/selection.vs", "Data/Shaders/selection.fs" });
	brush_shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2& new_position) {
	const glm::vec2 center_position = (new_position + brush_offset) - size * granularity * 0.25f;
	position = glm::floor(center_position) - glm::ceil(brush_offset);
	if (!uv_offset_locked) {
		uv_offset = glm::abs((center_position - glm::vec2(position)) * 4.f);
	}
}

glm::vec2 Brush::get_position() const {
	return glm::vec2(position) + brush_offset + 1.f;
}

void Brush::set_size(const int new_size) {
	const int change = new_size - size;

	size = std::clamp(new_size, 0, 240);
	const int cells = std::ceil(((size * 2 + 1) * granularity + 3) / 4.f);
	brush.clear();
	brush.resize(cells * 4 * cells * 4, { 0, 0, 0, 0 });

	set_shape(shape);

	set_position(glm::vec2(position) + glm::vec2(uv_offset + new_size * granularity - change * granularity) * 0.25f);
}

void Brush::set_shape(const Shape new_shape) {
	shape = new_shape;

	const int cells = std::ceil(((size * 2 + 1) * granularity + 3) / 4.f);

	for (int i = 0; i < size * 2 + 1; i++) {
		for (int j = 0; j < size * 2 + 1; j++) {
			for (int k = 0; k < granularity; k++) {
				for (int l = 0; l < granularity; l++) {
					if (contains(i, j)) {
						brush[(j * granularity + l) * cells * 4 + i * granularity + k] = { 0, 255, 0, 128 };
					} else {
						brush[(j * granularity + l) * cells * 4 + i * granularity + k] = { 0, 0, 0, 0 };
					}
				}
			}
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

/// Whether the brush shape contains the point, Arguments in brush coordinates
bool Brush::contains(const int x, const int y) const {
	const int cells = std::ceil(((size * 2 + 1) * granularity + 3) / 4.f);

	switch (shape) {
		case Shape::square:
			return true;
		case Shape::circle: {
			float distance = (x - size) * (x - size);
			distance += (y - size) * (y - size);
			return distance <= size * size;
		}
		case Shape::diamond:
			return std::abs(x - size) + std::abs(y - size) <= size;
	}
	return true;
}

void Brush::increase_size(const int new_size) {
	set_size(size + new_size);
}
void Brush::decrease_size(const int new_size) {
	set_size(size - new_size);
}

void Brush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Space:
			mode = (mode == Mode::placement) ? Mode::selection : Mode::placement;
			break;
		case Qt::Key_Equal:
			increase_size(1);
			break;
		case Qt::Key_Minus:
			decrease_size(1);
			break;
	}
}

void Brush::mouse_move_event(QMouseEvent* event) {
	set_position(input_handler.mouse_world);

	//if (input_handler.key_pressed(Qt::Key_Control)) {

	//}

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
		} else {
			apply();
		}
	}
}

void Brush::mouse_press_event(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (mode == Mode::selection) {
		if (!selection_started) {
			selection_started = true;
			selection_start = input_handler.mouse_world;
		}
	} else {
		if (event->button() == Qt::LeftButton) {
			map.brush->apply();
			map.brush->apply_end();
		}
	}
}

void Brush::mouse_release_event(QMouseEvent* event) {
	if (mode == Mode::selection) {
		selection_started = false;
	}
}

void Brush::render() const {
	if (mode == Mode::selection) {
		render_selection();
	} else {
		render_brush();
	}
	render_selectionn();
}

void Brush::render_selection() const {
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

	// +3 for uv_offset so it can move sub terrain cell
	const int cells = std::ceil(((this->size * 2 + 1) * granularity + 3) / 4.f);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera->projection_view[0][0]);
	gl->glUniform2f(2, position.x, position.y);
	gl->glUniform2f(3, uv_offset.x, uv_offset.y);

	gl->glBindTextureUnit(0, map.terrain.ground_corner_height);
	gl->glBindTextureUnit(1, brush_texture);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, cells * cells);

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}