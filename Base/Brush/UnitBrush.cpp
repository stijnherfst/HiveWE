#include "UnitBrush.h"

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

UnitBrush::UnitBrush() : Brush() {
}

void UnitBrush::set_shape(const Shape new_shape) {
}

void UnitBrush::key_press_event(QKeyEvent* event) {
	if (event->modifiers() & Qt::KeypadModifier) {
		if (!event->isAutoRepeat()) {
			map->terrain_undo.new_undo_group();
			unit_state_undo = std::make_unique<UnitStateAction>();
			for (const auto& i : selections) {
				unit_state_undo->old_units.push_back(*i);
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
	}

	if (event->modifiers() & Qt::ControlModifier) {
		switch (event->key()) {
			case Qt::Key_A:
				selections = map->units.query_area({ 0.0, 0.0, static_cast<double>(map->terrain.width), static_cast<double>(map->terrain.height) });
				break;
			default:
				Brush::key_press_event(event);
		}
	} else {
		Brush::key_press_event(event);
	}
}

void UnitBrush::key_release_event(QKeyEvent* event) {
	if (!event->isAutoRepeat()) {
		if (unit_state_undo) {
			for (const auto& i : selections) {
				unit_state_undo->new_units.push_back(*i);
			}
			map->terrain_undo.add_undo_action(std::move(unit_state_undo));
		}
	}
}

//void UnitBrush::mouse_release_event(QMouseEvent* event) {
//	Brush::mouse_release_event(event);
//	//if (event->button() == Qt::LeftButton && mode == Mode::selection) {
//	//	selection_started = false;
//	//} else {
//	//}
//}

void UnitBrush::mouse_press_event(QMouseEvent* event) {

	gl->glBindFramebuffer(GL_FRAMEBUFFER, map->render_manager.color_picking_framebuffer);

	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	gl->glViewport(0, 0, map->render_manager.window_width, map->render_manager.window_height);

	if (event->button() == Qt::LeftButton && mode == Mode::selection && !event->modifiers() && input_handler.mouse.y > 0.f) {
		map->render_manager.colored_skinned_shader->use();
		for (int i = 0; i < map->units.units.size(); i++) {
			const Unit& unit = map->units.units[i];
			if (unit.id == "sloc") {
				continue;
			} // ToDo handle starting locations

			mdx::Extent& extent = unit.mesh->model->sequences[unit.skeleton.sequence_index].extent;
			if (camera->inside_frustrum(unit.matrix * glm::vec4(extent.minimum, 1.f), unit.matrix * glm::vec4(extent.maximum, 1.f))) {
				unit.mesh->render_color_coded(unit.skeleton, i + 1);
			}
		}

		glm::u8vec4 color;
		glReadPixels(input_handler.mouse.x, map->render_manager.window_height - input_handler.mouse.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

		const int index = color.r + (color.g << 8) + (color.b << 16);
		if (index != 0) {
			Unit& unit = map->units.units[index - 1];
			selections = { &unit };
			dragging = true;
			drag_x_offset = input_handler.mouse_world.x - unit.position.x;
			drag_y_offset = input_handler.mouse_world.y - unit.position.y;
			return;
		}
	}
	Brush::mouse_press_event(event);
}

void UnitBrush::mouse_move_event(QMouseEvent* event) {
	Brush::mouse_move_event(event);

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
			if (dragging) {
				if (!dragged) {
					dragged = true;
					map->terrain_undo.new_undo_group();
					unit_state_undo = std::make_unique<UnitStateAction>();
					for (const auto& i : selections) {
						unit_state_undo->old_units.push_back(*i);
					}
				}
				for (auto& i : selections) {
					i->position.x = input_handler.mouse_world.x - drag_x_offset;
					i->position.y = input_handler.mouse_world.y - drag_y_offset;
					i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y);
					i->update();
				}
			} else if (event->modifiers() & Qt::ControlModifier) {
				for (auto&& i : selections) {
					float target_rotation = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					if (target_rotation < 0) {
						target_rotation = (glm::pi<float>() + target_rotation) + glm::pi<float>();
					}

					i->angle = target_rotation;
					i->update();
				}
			} else if (selection_started) {
				const glm::vec2 size = glm::vec2(input_handler.mouse_world) - selection_start;
				selections = map->units.query_area({ selection_start.x, selection_start.y, size.x, size.y });
			}
		}
	}
}

void UnitBrush::mouse_release_event(QMouseEvent* event) {
	dragging = false;
	if (dragged) {
		dragged = false;
		for (const auto& i : selections) {
			unit_state_undo->new_units.push_back(*i);
		}
		map->terrain_undo.add_undo_action(std::move(unit_state_undo));
	}

	Brush::mouse_release_event(event);
}

void UnitBrush::delete_selection() {
	if (selections.size()) {

		// Undo/redo
		map->terrain_undo.new_undo_group();
		auto action = std::make_unique<UnitDeleteAction>();
		for (const auto& i : selections) {
			action->units.push_back(*i);
		}
		map->terrain_undo.add_undo_action(std::move(action));
		map->units.remove_units(selections);

		selections.clear();
	}
}

void UnitBrush::copy_selection() {
	clipboard.clear();

	// Mouse position is average location
	clipboard_free_placement = true;
	glm::vec3 average_position = {};
	for (const auto& i : selections) {
		clipboard.push_back(*i);
		average_position += i->position;
	}
	clipboard_mouse_position = average_position / static_cast<float>(clipboard.size());
}

void UnitBrush::cut_selection() {
	copy_selection();
	// Delete selection will add to the undo/redo tree
	delete_selection();
}

void UnitBrush::clear_selection() {
	selections.clear();
}

void UnitBrush::place_clipboard() {
	apply_begin();
	for (const auto& i : clipboard) {
		Unit& new_unit = map->units.add_unit(i);
		new_unit.creation_number = ++Unit::auto_increment;
		glm::vec3 final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);

		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y);

		new_unit.position = final_position;
		new_unit.update();
		unit_undo->units.push_back(new_unit);
	}
	apply_end();
}

void UnitBrush::apply_begin() {
	map->terrain_undo.new_undo_group();
	unit_undo = std::make_unique<UnitAddAction>();
}

void UnitBrush::apply() {
	if (id.empty()) {
		return;
	}

	Unit& new_unit = map->units.add_unit(id, input_handler.mouse_world);
	new_unit.angle = rotation;
	new_unit.update();
	new_unit.player = player_id;

	unit_undo->units.push_back(new_unit);

	if (random_rotation) {
		set_random_rotation();
	}
}

void UnitBrush::apply_end() {
	map->terrain_undo.add_undo_action(std::move(unit_undo));
}

void UnitBrush::render_brush() {
	const float model_scale = units_slk.data<float>("modelscale", id);
	const float move_height = units_slk.data<float>("moveheight", id);

	glm::vec3 final_position = input_handler.mouse_world;
	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y) + move_height / 128.f;
	const glm::vec3 final_scale = glm::vec3(model_scale / 128.f);

	if (mesh) {
		skeleton.updateLocation(final_position, rotation, final_scale);
		skeleton.update(0.016f);
		mesh->render_queue(skeleton, glm::vec3(1.f));
	}
}

void UnitBrush::render_selection() const {
	gl->glDisable(GL_DEPTH_TEST);
	selection_circle_shader->use();
	gl->glEnableVertexAttribArray(0);

	for (const auto& i : selections) {
		float selection_scale = i->mesh->model->sequences[i->skeleton.sequence_index].extent.bounds_radius / 128.f;

		glm::mat4 model(1.f);
		model = glm::translate(model, i->position - glm::vec3(selection_scale * 0.5f, selection_scale * 0.5f, 0.f));
		model = glm::scale(model, glm::vec3(selection_scale));

		model = camera->projection_view * model;
		gl->glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
		gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}

void UnitBrush::render_clipboard() {
	for (auto& i : clipboard) {
		const float model_scale = units_slk.data<float>("modelscale", i.id);
		const float move_height = units_slk.data<float>("moveheight", i.id);

		glm::vec3 final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);
		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y) + move_height / 128.f;

		const glm::vec3 final_scale = glm::vec3(model_scale / 128.f);

		i.skeleton.updateLocation(final_position, i.angle, final_scale);
		i.skeleton.update(0.016f);
		i.mesh->render_queue(i.skeleton, glm::vec3(1.f));
	}
}

void UnitBrush::set_random_rotation() {
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
	rotation = dist(gen);
}

void UnitBrush::set_unit(const std::string& id) {
	context->makeCurrent();
	this->id = id;
	mesh = map->units.get_mesh(id);
	skeleton = SkeletalModelInstance(mesh->model);
}