#include "unit_brush.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <random>
#include <memory>

#include "globals.h"
#include <map_global.h>

import Hierarchy;
import Texture;
import TerrainUndo;
import Camera;
import OpenGLUtilities;
import RenderManager;

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
				selections.clear();
				selections.reserve(map->units.units.size());
				for (auto& i : map->units.units) {
					if (i.id == "sloc") {
						continue;
					}
					selections.emplace(&i);
				}
				emit selection_changed();
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

void UnitBrush::mouse_press_event(QMouseEvent* event, double frame_delta) {
	// The mouse.y check is needed as sometimes it is negative for unknown reasons
	if (event->button() == Qt::LeftButton && input_handler.mouse.y > 0.f) {
		if (mode == Mode::selection) {
			if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier) {
				auto id = map->render_manager.pick_unit_id_under_mouse(map->units, input_handler.mouse);
				if (id) {
					if (selections.contains(&map->units.units[id.value()])) {
						selections.erase(&map->units.units[id.value()]);
					} else {
						selections.emplace(&map->units.units[id.value()]);
					}
					return;
				}
			}

			if (!event->modifiers()) {
				auto id = map->render_manager.pick_unit_id_under_mouse(map->units, input_handler.mouse);
				if (id) {
					Unit& unit = map->units.units[id.value()];

					drag_start = input_handler.mouse_world;
					dragging = true;

					// If the current index is already in a selection then we want to drag the entire group
					if (std::find(selections.begin(), selections.end(), &unit) != selections.end()) {
						drag_offsets.clear();
						for (const auto& i : selections) {
							drag_offsets.push_back(input_handler.mouse_world - i->position);
						}
					} else {
						selections = { &unit };
						drag_offsets = { input_handler.mouse_world - unit.position };
						emit selection_changed();
					}
					return;
				}
			}
		}
	}

	Brush::mouse_press_event(event, frame_delta);
}

void UnitBrush::mouse_move_event(QMouseEvent* event, double frame_delta) {
	Brush::mouse_move_event(event,	frame_delta);

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

				glm::vec3 offset = input_handler.mouse_world - drag_start;
				offset.z = 0;

				drag_start = input_handler.mouse_world;

				for (const auto& unit : selections) {
					unit->position += offset;
					unit->position.z = map->terrain.interpolated_height(unit->position.x, unit->position.y, true);
					unit->update();
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

				auto query = map->units.query_area({ selection_start.x, selection_start.y, size.x, size.y });
				if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier) {
					selections.insert(query.begin(), query.end());
				} else if (event->modifiers() & Qt::KeyboardModifier::AltModifier) {
					for (const auto& i : query) {
						selections.erase(i);
					}
				} else {
					selections.clear();
					selections.insert(query.begin(), query.end());
				}


				//selections = map->units.query_area({ selection_start.x, selection_start.y, size.x, size.y });
				emit selection_changed();
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
	if (!selections.size()) {
		return;
	}

	// Undo/redo
	map->terrain_undo.new_undo_group();
	auto action = std::make_unique<UnitDeleteAction>();
	for (const auto& i : selections) {
		action->units.push_back(*i);
	}
	map->terrain_undo.add_undo_action(std::move(action));
	map->units.remove_units(selections);

	selections.clear();
	emit selection_changed();
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
	clipboard_mouse_offset = average_position / static_cast<float>(clipboard.size());
}

void UnitBrush::cut_selection() {
	copy_selection();
	// Delete selection will add to the undo/redo tree
	delete_selection();
}

void UnitBrush::clear_selection() {
	selections.clear();
	emit selection_changed();
}

void UnitBrush::place_clipboard() {
	apply_begin();
	for (const auto& i : clipboard) {
		Unit& new_unit = map->units.add_unit(i);
		new_unit.creation_number = ++Unit::auto_increment;
		glm::vec3 final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_offset, 0);

		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true);

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

void UnitBrush::apply(double frame_delta) {
	if (id.empty()) {
		return;
	}

	glm::vec3 final_position = input_handler.mouse_world;
	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true);

	Unit& new_unit = map->units.add_unit(id, final_position);
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
	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true) + move_height / 128.f;
	const glm::vec3 final_scale = glm::vec3(model_scale / 128.f);

	if (mesh) {
		skeleton.update_location(final_position, glm::angleAxis(rotation, glm::vec3(0, 0, 1)), final_scale);
		skeleton.update(0.016f);
		map->render_manager.queue_render(*mesh, skeleton, glm::vec3(1.f));
	}
}

void UnitBrush::render_selection() const {
	glDisable(GL_DEPTH_TEST);
	selection_circle_shader->use();
	glEnableVertexAttribArray(0);

	for (const auto& i : selections) {
		float selection_scale = i->mesh->model->sequences[i->skeleton.sequence_index].extent.bounds_radius / 128.f;

		glm::mat4 model(1.f);
		model = glm::translate(model, i->position - glm::vec3(selection_scale * 0.5f, selection_scale * 0.5f, 0.f));
		model = glm::scale(model, glm::vec3(selection_scale));

		model = camera.projection_view * model;
		glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	glDisableVertexAttribArray(0);
	glEnable(GL_DEPTH_TEST);
}

void UnitBrush::render_clipboard() {
	for (auto& i : clipboard) {
		const float model_scale = units_slk.data<float>("modelscale", i.id);
		const float move_height = units_slk.data<float>("moveheight", i.id);

		glm::vec3 final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_offset, 0);
		final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true) + move_height / 128.f;

		const glm::vec3 final_scale = glm::vec3(model_scale / 128.f);

		i.skeleton.update_location(final_position, glm::angleAxis(i.angle, glm::vec3(0, 0, 1)), final_scale);
		i.skeleton.update(0.016f);
		//i.mesh->render_queue(i.skeleton, glm::vec3(1.f));
		map->render_manager.queue_render(*i.mesh, i.skeleton, glm::vec3(1.f));
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

void UnitBrush::unselect_id(std::string_view id) {
	if (this->id == id) {
		set_unit("hfoo");
	}
}