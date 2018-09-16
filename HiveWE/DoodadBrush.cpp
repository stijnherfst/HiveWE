#include "stdafx.h"

void DoodadBrush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Delete:
			if (selections.size()) {
				map.doodads.remove_doodads(selections);
				selections.clear();
			}
			break;
		case Qt::Key_A:
			if (event->modifiers() & Qt::ControlModifier) {
				selections = map.doodads.query_area({0.0, 0.0, static_cast<double>(map.terrain.width), static_cast<double>(map.terrain.height)});
			}
			break;
		default:
			Brush::key_press_event(event);
	}
}

void DoodadBrush::mouse_release_event(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && mode == Mode::selection) {
		selection_started = false;
	}
}

void DoodadBrush::mouse_move_event(QMouseEvent* event) {
	Brush::mouse_move_event(event);

	if (mode == Mode::selection) {
		if (event->buttons() == Qt::LeftButton) {
			if (event->modifiers() & Qt::ControlModifier) {
				for (auto&& i : selections) {
					i->angle = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					i->matrix = glm::translate(glm::mat4(1.f), i->position);
					i->matrix = glm::scale(i->matrix, i->scale);
					i->matrix = glm::rotate(i->matrix, i->angle, glm::vec3(0, 0, 1));
				}
			} else {
				glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
				selections = map.doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });
			}
		}
	}
}

void DoodadBrush::apply() {
	if (id == "") {
		return;
	}
	map.doodads.add_doodad(id, free_placement ? input_handler.mouse_world : glm::vec3(glm::vec2(glm::ivec2(input_handler.mouse_world * 2.f)) * 0.5f + 0.25f, input_handler.mouse_world.z));
}

void DoodadBrush::render_brush() const {
	//int cells = 
	//for ()


	glm::mat4 mat(1.f);
	if (free_placement) {
		mat = glm::translate(mat, input_handler.mouse_world);
	} else {
		mat = glm::translate(mat, glm::vec3(glm::vec2(glm::ivec2(input_handler.mouse_world * 2.f)) * 0.5f + 0.25f, input_handler.mouse_world.z));
	}
	mat = glm::scale(mat, glm::vec3(1.f / 128.f));

	if (mesh) {
		mesh->render_queue(mat);
	}
}

void DoodadBrush::render_selection() const {
	gl->glDisable(GL_DEPTH_TEST);
	selection_shader->use();
	gl->glEnableVertexAttribArray(0);

	for (auto&& i : selections) {
		glm::mat4 model(1.f);
		model = glm::translate(model, i->position - glm::vec3(0.5f, 0.5f, 0.f));
		model = camera->projection_view * model;
		gl->glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		gl->glDrawArrays(GL_LINE_LOOP, 0, 4);
	}

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}

void DoodadBrush::set_doodad(const std::string& id) {
	this->id = id;

	bool is_doodad = map.doodads.doodads_slk.row_header_exists(id);
	slk::SLK& slk = is_doodad ? map.doodads.doodads_slk : map.doodads.destructibles_slk;

	std::string pathing_texture_path = slk.data("pathTex", id);

	if (pathing_texture_path.empty()) {
		free_placement = true;
		free_rotation = true;
	} else {
		if (hierarchy.file_exists(pathing_texture_path)) {
			free_placement = false;
			auto pathing_texture = resource_manager.load<Texture>(pathing_texture_path);
			free_rotation = pathing_texture->width == pathing_texture->height;
		} else {
			free_placement = true;
		}
	}

	mesh = map.doodads.get_mesh(id, 0);
}