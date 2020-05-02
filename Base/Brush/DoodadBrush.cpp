#include "DoodadBrush.h"

#include <random>
#include <memory>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "InputHandler.h"
#include "TerrainUndo.h"
#include "HiveWE.h"
#include "Hierarchy.h"
#include "Texture.h"

DoodadBrush::DoodadBrush() : Brush() {
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
		const int div_w = (((int)glm::degrees(rotation) + 90) % 180) ? pathing_texture->height : pathing_texture->width;
		const int div_h = (((int)glm::degrees(rotation) + 90) % 180) ? pathing_texture->width : pathing_texture->height;

		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				brush[j * size + i] = { 0, 0, 0, 0 };
			}
		}

		for (int i = 0; i < pathing_texture->width; i++) {
			for (int j = 0; j < pathing_texture->height; j++) {
				int x = i;
				int y = j;

				switch (((int)glm::degrees(rotation) + 90) % 360) {
					case 90:
						x = pathing_texture->height - 1 - j;
						y = i;
						break;
					case 180:
						x = pathing_texture->width - 1 - i;
						y = pathing_texture->height - 1 - j;
						break;
					case 270:
						x = j;
						y = pathing_texture->width - 1 - i;
						break;
				}

				const int in = ((pathing_texture->height - 1 - j) * pathing_texture->width + i) * pathing_texture->channels;
				const int index = (y + std::max(0, div_w - div_h) / 2) * size + x + std::max(0, div_h - div_w) / 2;

				// Have to check for > 250 because sometimes the pathing textures are not properly thresholded
				glm::vec4 color = { pathing_texture->data[in + 2] > 250 ? 255 : 0,
					pathing_texture->data[in + 1] > 250 ? 255 : 0,
					pathing_texture->data[in] > 250 ? 255 : 0,
					128 };

				if (color.r || color.g || color.b) {
					brush[index] = color;
				}
			}
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

void DoodadBrush::key_press_event(QKeyEvent* event) {
	if (event->modifiers() & Qt::KeypadModifier) {
		if (!event->isAutoRepeat()) {
			map->terrain_undo.new_undo_group();
			doodad_state_undo = std::make_unique<DoodadStateAction>();
			for (const auto& i : selections) {
				doodad_state_undo->old_doodads.push_back(*i);
			}
		}

		bool left = event->key() == Qt::Key_1 || event->key() == Qt::Key_4 || event->key() == Qt::Key_7;
		bool right = event->key() == Qt::Key_3 || event->key() == Qt::Key_6 || event->key() == Qt::Key_9;
		bool down = event->key() == Qt::Key_7 || event->key() == Qt::Key_8 || event->key() == Qt::Key_9;
		bool up = event->key() == Qt::Key_1 || event->key() == Qt::Key_2 || event->key() == Qt::Key_3;

		float x_displacement = -0.25f * left + 0.25f * right;
		float y_displacement = -0.25f * up + 0.25f * down;

		for (const auto& i : selections) {
			i->position.x += x_displacement;
			i->position.y += y_displacement;
			i->update();
		}

		map->doodads.update_doodad_pathing(selections);
	}

	if (event->modifiers() & Qt::ControlModifier) {
		switch (event->key()) {
			case Qt::Key_A:
					selections = map->doodads.query_area({0.0, 0.0, static_cast<double>(map->terrain.width), static_cast<double>(map->terrain.height)});
				break;
			case Qt::Key_PageUp:
				for (const auto& i : selections) {
					i->position.z += 0.1f;
					i->update();
				}
				break;
			case Qt::Key_PageDown:
				for (const auto& i : selections) {
					i->position.z -= 0.1f;
					i->update();
				}
				break;
			default:
				Brush::key_press_event(event);
		}
	} else {
		switch (event->key()) {
			case Qt::Key_PageUp:
				for (const auto& i : selections) {
					i->scale.z += 0.1f;
					i->update();
				}
				break;
			case Qt::Key_PageDown:
				for (const auto& i : selections) {
					i->scale.z -= 0.1f;
					i->update();
				}
				break;
			default:
				Brush::key_press_event(event);
		}
	}
}

void DoodadBrush::key_release_event(QKeyEvent* event) {
	if (!event->isAutoRepeat()) {
		if (doodad_state_undo) {
			for (const auto& i : selections) {
				doodad_state_undo->new_doodads.push_back(*i);
			}
			map->terrain_undo.add_undo_action(std::move(doodad_state_undo));
		}
		//doodad_state_undo
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
					bool fixed_rotation = false;
					if (doodads_slk.row_header_exists(i->id)) {
						fixed_rotation = doodads_slk.data<int>("fixedrot", i->id) > 0;
					} else {
						fixed_rotation = destructables_slk.data<int>("fixedrot", i->id) > 0;
					}

					if (fixed_rotation) {
						continue; 
					}

					float target_rotation = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					if (target_rotation < 0) {
						target_rotation = (glm::pi<float>() + target_rotation) + glm::pi<float>();
					}

					if (i->pathing->width == i->pathing->height) {
						if (i->pathing->homogeneous) {
							i->angle = target_rotation;

						} else {
							i->angle = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4) * glm::pi<float>() * 0.5f;
						}
					} else {
						i->angle = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4)* glm::pi<float>() * 0.5f;
					}
					i->update();
				}

				map->doodads.update_doodad_pathing(selections);
			} else if (mode == Mode::selection && selection_started) {
				const glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
				selections = map->doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });

				// If we should remove doodads/destructibles from the selection
				/*if (!select_doodads || !select_destructibles) {
					for (int i = selections.size(); i-- > 0;) {
						if (doodads_slk.row_header_exists(selections[i]->id)) {
							if (!select_doodads) {

							}
						} else {
							if (!select_destructibles) {

							}
						}
					}
				}*/
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

		map->doodads.update_doodad_pathing(update_pathing_area);

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

	if (random_rotation) {
		set_random_rotation();
		set_shape(shape);
	}

	if (random_variation) {
		set_random_variation();
	}

	if (random_scale) {
		std::random_device rd;
		std::mt19937 gen(rd());
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
	matrix = glm::rotate(matrix, roll, glm::vec3(1, 0, 0));

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
			base_scale = glm::vec3(doodads_slk.data<float>("defscale", i.id));
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

void DoodadBrush::set_random_rotation() {
	std::random_device rd;
	std::mt19937 gen(rd());

	bool fixed_rotation = false;
	if (doodads_slk.row_header_exists(id)) {
		fixed_rotation = doodads_slk.data<int>("fixedrot", id) > 0;
	} else {
		fixed_rotation = destructables_slk.data<int>("fixedrot", id) > 0;
	}

	std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
	float target_rotation = dist(gen);
	if (pathing_texture && pathing_texture->width == pathing_texture->height) {
		if (pathing_texture->homogeneous) {
			rotation = target_rotation;
		} else {
			rotation = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4)* glm::pi<float>() * 0.5f;
		}
	} else {
		rotation = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4)* glm::pi<float>() * 0.5f;
	}
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
	set_random_variation();

	const bool is_doodad = doodads_slk.row_header_exists(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructables_slk;
	
	min_scale = slk.data<float>("minscale", id);
	max_scale = slk.data<float>("maxscale", id);

	std::string maxRoll = doodads_slk.data("maxroll", id);
	if (!maxRoll.empty()) {
		roll = -std::stof(maxRoll);
	} else{
		roll = 0;
	}

	if (is_doodad) {
		scale = slk.data<float>("defscale", id);
	}

	if (slk.data<int>("fixedrot", id) < 0) {
		rotation = glm::pi<float>() * 1.5f;
	} else {
		rotation = glm::radians(slk.data<float>("fixedrot", id));
	}

	pathing_texture.reset();
	std::string pathing_texture_path = slk.data("pathtex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		free_placement = false;
		pathing_texture = resource_manager.load<PathingTexture>(pathing_texture_path);

		set_size(std::max(pathing_texture->width, pathing_texture->height));

		free_rotation = pathing_texture->width == pathing_texture->height;
		free_rotation = free_rotation && slk.data<float>("fixedrot", id) < 0.f;
	} else {
		free_placement = true;
		free_rotation = true;
	}

	possible_variations.clear();
	int variation_count = slk.data<int>("numvar", id);
	for (int i = 0; i < variation_count; i++) {
		possible_variations.insert(i);
	}
}