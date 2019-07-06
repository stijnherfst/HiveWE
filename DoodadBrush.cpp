#include "stdafx.h"

DoodadBrush::DoodadBrush() {
	uv_offset_granularity = 2;
	brush_offset = { 0.25f , 0.25f };
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

	if (pathing_texture) {
		for (int i = 0; i < pathing_texture->width; i++) {
			for (int j = 0; j < pathing_texture->height; j++) {
				int x = i;
				int y = j;

				switch ((int)glm::degrees(rotation) + 90) {
					case 90:
						x = pathing_texture->width - 1 - j - std::max(0, pathing_texture->width - pathing_texture->height);
						y = i + std::max(0, pathing_texture->height - pathing_texture->width);
						break;
					case 180:
						x = pathing_texture->width - 1 - i;
						y = pathing_texture->height - 1 - j;
						break;
					case 270:
						x = j + std::max(0, pathing_texture->height - pathing_texture->width);
						y = pathing_texture->height - 1 - i + std::max(0, pathing_texture->width - pathing_texture->height);
						break;
				}

				const int in = ((pathing_texture->height - 1 - j) * pathing_texture->width + i) * pathing_texture->channels;
				

				const int div_w = (((int)glm::degrees(rotation) + 90) % 180) ? pathing_texture->height : pathing_texture->width;
				const int div_h = (((int)glm::degrees(rotation) + 90) % 180) ? pathing_texture->width : pathing_texture->height;
				const int index = (y + std::max(0, div_w - div_h) / 2) * size + x + std::max(0, div_h - div_w) / 2;

				glm::vec4 color = { pathing_texture->data[in + 2] > 250 ? 255 : 0,
					pathing_texture->data[in + 1] > 250 ? 255 : 0,
					pathing_texture->data[in] > 250 ? 255 : 0,
					128 };

				if (color.r || color.g || color.b) {
					brush[index] = color;
				} else {
					brush[index] = { 0, 0, 0, 0 };
				}
			}
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

void DoodadBrush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
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
	} else {
		Brush::mouse_release_event(event);
	}
}

void DoodadBrush::mouse_move_event(QMouseEvent* event) {
	Brush::mouse_move_event(event);

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
			if (event->modifiers() & Qt::ControlModifier) {
				for (auto&& i : selections) {
					i->angle = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					i->update();
				}
			} else if (mode == Mode::selection && selection_started) {
				const glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
				selections = map->doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });
			}
		}
	}
}

void DoodadBrush::delete_selection() {
	if (selections.size()) {
		QRectF update_pathing_area;
		// Undo/redo
		map->terrain_undo.new_undo_group();
		auto action = std::make_unique<DoodadDeleteAction>();
		for (const auto& i : selections) {
			action->doodads.push_back(*i);

			if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
				update_pathing_area = { i->position.x, i->position.y, 1.f, 1.f };
			}
			update_pathing_area |= { i->position.x, i->position.y, 1.f, 1.f };
		}
		map->terrain_undo.add_undo_action(std::move(action));
		map->doodads.remove_doodads(selections);

		// Update pathing
		update_pathing_area.adjust(-6, -6, 6, 6);
		map->pathing_map.dynamic_clear_area(update_pathing_area.toRect());

		update_pathing_area.adjust(-6, -6, 6, 6);

		const auto doodads_to_blit = map->doodads.query_area(update_pathing_area);
		for (const auto& i : doodads_to_blit) {
			if (!i->pathing) {
				continue;
			}
			map->pathing_map.blit_pathing_texture(i->position, 0, i->pathing);
		}
		map->pathing_map.upload_dynamic_pathing();

		selections.clear();
	}
}

void DoodadBrush::copy_selection() {
	clipboard.clear();

	// Mouse position is average location
	clipboard_free_placement = true;
	glm::vec3 average_position = {};
	for (const auto& i : selections) {
		if (i->pathing) {
			clipboard_free_placement = false;
		}
		clipboard.push_back(*i);
		average_position += i->position;
	}
	clipboard_mouse_position = average_position / static_cast<float>(clipboard.size());
}

void DoodadBrush::cut_selection() {
	copy_selection();
	// Delete selection will add to the undo/redo tree
	delete_selection();
}

void DoodadBrush::clear_selection() {
	selections.clear();
}

void DoodadBrush::place_clipboard() {
	apply_begin();
	for (const auto& i : clipboard) {
		Doodad& new_doodad = map->doodads.add_doodad(i);

		glm::vec3 final_position;
		if (clipboard_free_placement) {
			final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);
		} else {
			final_position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f - glm::vec2(glm::ivec2(clipboard_mouse_position * 4.f)) / 4.f, 0) + i.position;
		}
		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y);

		new_doodad.position = final_position;
		new_doodad.update();
		doodad_undo->doodads.push_back(new_doodad);
		
		if (new_doodad.pathing) {
			map->pathing_map.blit_pathing_texture(new_doodad.position, glm::degrees(rotation) + 90, new_doodad.pathing);
		}
	}
	map->pathing_map.upload_dynamic_pathing();
	apply_end();
}

void DoodadBrush::apply_begin() {
	map->terrain_undo.new_undo_group();
	doodad_undo = std::make_unique<DoodadAddAction>();
}

void DoodadBrush::apply() {
	if (id == "") {
		return;
	}

	glm::vec3 doodad_position;
	if (free_placement) {
		doodad_position = input_handler.mouse_world;
	} else {
		doodad_position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f, input_handler.mouse_world.z);
	}

	Doodad& doodad = map->doodads.add_doodad(id, variation, doodad_position);
	doodad.scale = glm::vec3(scale);
	doodad.angle = rotation;
	doodad.state = state;
	doodad.update();

	doodad_undo->doodads.push_back(doodad);

	if (pathing_texture) {
		map->pathing_map.blit_pathing_texture(doodad_position, glm::degrees(rotation) + 90, pathing_texture);
		map->pathing_map.upload_dynamic_pathing();
	}

	// Setup for the next apply
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

void DoodadBrush::apply_end() {
	map->terrain_undo.add_undo_action(std::move(doodad_undo));
}

void DoodadBrush::render_brush() const {
	if (pathing_texture) {
		Brush::render_brush();
	}

	glm::mat4 matrix(1.f);
	if (free_placement) {
		matrix = glm::translate(matrix, input_handler.mouse_world);
	} else {
		matrix = glm::translate(matrix, glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f, input_handler.mouse_world.z));
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

	for (const auto& i : selections) {
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

void DoodadBrush::render_clipboard() const {
	for (const auto& i : clipboard) {
		glm::vec3 base_scale = glm::vec3(1.f);

		if (doodads_slk.row_header_exists(i.id)) {
			base_scale = glm::vec3(doodads_slk.data<float>("defScale", i.id));
		}

		glm::vec3 final_position;
		if (clipboard_free_placement) {
			final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);
		} else {
			// clipboard_mouse_position only per 0.5 units?
			final_position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f - glm::vec2(glm::ivec2(clipboard_mouse_position * 2.f)) / 2.f, 0) + i.position;
		}
		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y);

		glm::mat4 model(1.f);
		model = glm::translate(model, final_position);
		model = glm::scale(model, (base_scale - 1.f + i.scale) / 128.f);
		model = glm::rotate(model, i.angle, glm::vec3(0, 0, 1));

		i.mesh->render_queue(model);
	}
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
	const bool is_doodad = doodads_slk.row_header_exists(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;


	min_scale = slk.data<float>("minScale", id);
	max_scale = slk.data<float>("maxScale", id);

	if (is_doodad) {
		scale = slk.data<float>("defScale", id);
	}

	if (slk.data<int>("fixedRot", id) == -1) {
		rotation = glm::pi<float>() * 1.5f;
	} else {
		rotation = glm::radians(slk.data<float>("fixedRot", id));
	}

	pathing_texture.reset();
	std::string pathing_texture_path = slk.data("pathTex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		free_placement = false;
		pathing_texture = resource_manager.load<Texture>(pathing_texture_path);

		set_size(std::max(pathing_texture->width, pathing_texture->height));

		free_rotation = pathing_texture->width == pathing_texture->height;
		free_rotation = free_rotation && slk.data<float>("fixedRot", id) < 0.f;
	} else {
		free_placement = true;
		free_rotation = true;
	}

	possible_variations.clear();
	int variation_count = slk.data<int>("numVar", id);
	for (int i = 0; i < variation_count; i++) {
		possible_variations.insert(i);
	}
}