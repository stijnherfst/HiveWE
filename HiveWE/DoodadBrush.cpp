#include "stdafx.h"

/// Gets a random variation from the possible_variation list
int DoodadBrush::get_random_variation() {
	if (possible_variations.size() == 0) {
		return 0;
	}

	std::mt19937 rng;
	rng.seed(std::random_device()());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, possible_variations.size() - 1);

	auto it = possible_variations.begin();
	std::advance(it, dist(rng));
	return *it;
}

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

	if (mode == Mode::selection && selection_started) {
		if (event->buttons() == Qt::LeftButton) {
			if (event->modifiers() & Qt::ControlModifier) {
				for (auto&& i : selections) {
					i->angle = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					i->update();
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

	glm::vec3 position;
	if (free_placement) {
		position = input_handler.mouse_world;
	} else {
		// Round to 0.5
		position = glm::vec3(glm::trunc(glm::vec2(input_handler.mouse_world) * 2.f) * 0.5f + 0.25f, input_handler.mouse_world.z);
	}

	Doodad& doodad = map.doodads.add_doodad(id, variation, position);
	doodad.scale = { scale, scale, scale };
	doodad.angle = rotation;
	doodad.update();

	std::random_device rd;
	std::mt19937 gen(rd());

	if (random_rotation && free_rotation) {
		std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
		rotation = dist(gen);
		rotation = 0.f;
	}

	if (random_variation) {
		set_random_variation();
	}

	if (random_scale) {
		std::uniform_real_distribution dist(min_scale, max_scale);
		scale = dist(gen);
	}
}

void DoodadBrush::render_brush() const {
	//int cells = 
	//for ()


	glm::mat4 matrix(1.f);
	if (free_placement) {
		matrix = glm::translate(matrix, input_handler.mouse_world);
	} else {
		matrix = glm::translate(matrix, glm::vec3(glm::vec2(glm::ivec2(input_handler.mouse_world * 2.f)) * 0.5f + 0.25f, input_handler.mouse_world.z));
	}
	matrix = glm::scale(matrix, glm::vec3(1.f / 128.f) * scale);
	matrix = glm::rotate(matrix, rotation, glm::vec3(0, 0, 1));

	if (mesh) {
		mesh->render_queue(matrix);
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

void DoodadBrush::set_random_variation() {
	variation = get_random_variation();
	mesh = map.doodads.get_mesh(id, variation);
}

void DoodadBrush::add_variation(int variation) {
	possible_variations.insert(variation);
}

void DoodadBrush::erase_variation(int variation) {
	possible_variations.erase(variation);
	if (this->variation == variation) {
		this->variation = get_random_variation();
	}
}

void DoodadBrush::set_doodad(const std::string& id) {
	this->id = id;
	if (random_variation) {
		set_random_variation();
	}
	bool is_doodad = map.doodads.doodads_slk.row_header_exists(id);
	slk::SLK& slk = is_doodad ? map.doodads.doodads_slk : map.doodads.destructibles_slk;


	min_scale = std::stof(slk.data("minScale", id));
	max_scale = std::stof(slk.data("maxScale", id));

	if (is_doodad) {
		scale = std::stof(slk.data("defScale", id));
	}

	rotation = glm::radians(std::max(0.f, std::stof(slk.data("fixedRot", id))));
	//rotation = 0.f;

	std::string pathing_texture_path = slk.data("pathTex", id);
	if (pathing_texture_path.empty()) {
		free_placement = true;
		free_rotation = true;
	} else {
		if (hierarchy.file_exists(pathing_texture_path)) {
			free_placement = false;
			pathing_texture = resource_manager.load<Texture>(pathing_texture_path);
			free_rotation = pathing_texture->width == pathing_texture->height;
			free_rotation = free_rotation && std::stof(slk.data("fixedRot", id)) < 0.f;
		} else {
			free_placement = true;
		}
	}

	possible_variations.clear();
	int variation_count = std::stoi(slk.data("numVar", id));
	for (int i = 0; i < variation_count; i++) {
		possible_variations.insert(i);
	}
}