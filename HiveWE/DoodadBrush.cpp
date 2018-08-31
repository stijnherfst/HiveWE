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
		
			break;
		default:
			Brush::key_press_event(event);
	}
}

void DoodadBrush::mouse_release_event(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && mode == Mode::selection) {
		selection_started = false;
		glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
		selections = map.doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });
	}
}

void DoodadBrush::apply() {
	if (id == "") {
		return;
	}
	map.doodads.add_doodad(id, free_placement ? input_handler.mouse_world : get_position());
}

void DoodadBrush::render_brush() const {
	glm::mat4 mat(1.f);
	if (free_placement) {
		mat = glm::translate(mat, input_handler.mouse_world);
	} else {
		mat = glm::translate(mat, glm::vec3(get_position(), 0));
	}
	mat = glm::scale(mat, glm::vec3(1.f / 128.f));

	if (mesh) {
		mesh->render_queue(mat);
	}
}

void DoodadBrush::render_selectionn() const {
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
		free_placement = false;
		if (hierarchy.file_exists(pathing_texture_path)) {
			auto pathing_texture = resource_manager.load<Texture>(pathing_texture_path);
			free_rotation = pathing_texture->width == pathing_texture->height;
		} else {
			free_rotation = false;
		}
	}

	mesh = map.doodads.get_mesh(id, 0);
}