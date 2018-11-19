#include "stdafx.h"

DoodadBrush::DoodadBrush() {
	size_granularity = 2;
	uv_offset_granularity = 2;
}

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

void DoodadBrush::set_shape(const Shape new_shape) {
	shape = new_shape;

	const int cells = std::ceil(((size * 2 + 1) * size_granularity + 3) / 4.f);

	/*for (int i = 0; i < size * 2 + 1; i++) {
		for (int j = 0; j < size * 2 + 1; j++) {
			for (int k = 0; k < size_granularity; k++) {
				for (int l = 0; l < size_granularity; l++) {
					if (contains(i, j)) {
						brush[(j * size_granularity + l) * cells * 4 + i * size_granularity + k] = brush_color;
					} else {
						brush[(j * size_granularity + l) * cells * 4 + i * size_granularity + k] = { 0, 0, 0, 0 };
					}
				}
			}
		}
	}*/

	if (pathing_texture) {

		for (int i = 0; i < pathing_texture->width; i++) {
			for (int j = 0; j < pathing_texture->height; j++) {
//				brush[j * pathing_texture->width + i] = pathing_texture->data[j * pathing_texture->width + i];
			}
		}
	} else {
		puts("\n");
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

void DoodadBrush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Delete:
			if (selections.size()) {
				map->doodads.remove_doodads(selections);
				selections.clear();
			}
			break;
		case Qt::Key_A:
			if (event->modifiers() & Qt::ControlModifier) {
				selections = map->doodads.query_area({0.0, 0.0, static_cast<double>(map->terrain.width), static_cast<double>(map->terrain.height)});
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
				selections = map->doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });
			}
		}
	}
}

void DoodadBrush::clear_selection() {
	selections.clear();
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

	Doodad& doodad = map->doodads.add_doodad(id, variation, position);
	doodad.scale = glm::vec3(scale);
	doodad.angle = rotation;
	doodad.state = state;
	doodad.update();

	std::random_device rd;
	std::mt19937 gen(rd());

	if (random_rotation && free_rotation) {
		std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
		rotation = dist(gen);
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
	Brush::render_brush();

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
	mesh = map->doodads.get_mesh(id, variation);
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
	bool is_doodad = doodads_slk.row_header_exists(id);
	slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;


	min_scale = slk.data<float>("minScale", id);
	max_scale = slk.data<float>("maxScale", id);

	if (is_doodad) {
		scale = slk.data<float>("defScale", id);
	}

	rotation = glm::radians(std::max(0.f, slk.data<float>("fixedRot", id)));

	std::string pathing_texture_path = slk.data("pathTex", id);
	if (pathing_texture_path.empty() || !hierarchy.file_exists(pathing_texture_path)) {
		free_placement = true;
		free_rotation = true;
	} else {
		if (hierarchy.file_exists(pathing_texture_path)) {
			free_placement = false;
			pathing_texture = resource_manager.load<Texture>(pathing_texture_path);
			set_size(pathing_texture->width);

			free_rotation = pathing_texture->width == pathing_texture->height;
			free_rotation = free_rotation && slk.data<float>("fixedRot", id) < 0.f;
		} else {
			free_placement = true;
		}
	}

	possible_variations.clear();
	int variation_count = slk.data<int>("numVar", id);
	for (int i = 0; i < variation_count; i++) {
		possible_variations.insert(i);
	}
}