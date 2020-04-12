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

UnitBrush::UnitBrush() {

}

void UnitBrush::set_shape(const Shape new_shape) {

}

void UnitBrush::key_press_event(QKeyEvent* event) {
	//if (event->modifiers() & Qt::KeypadModifier) {
	//	if (!event->isAutoRepeat()) {
	//		map->terrain_undo.new_undo_group();
	//		doodad_state_undo = std::make_unique<DoodadStateAction>();
	//		for (const auto& i : selections) {
	//			doodad_state_undo->old_doodads.push_back(*i);
	//		}
	//	}

	//	bool left = event->key() == Qt::Key_1 || event->key() == Qt::Key_4 || event->key() == Qt::Key_7;
	//	bool right = event->key() == Qt::Key_3 || event->key() == Qt::Key_6 || event->key() == Qt::Key_9;
	//	bool down = event->key() == Qt::Key_7 || event->key() == Qt::Key_8 || event->key() == Qt::Key_9;
	//	bool up = event->key() == Qt::Key_1 || event->key() == Qt::Key_2 || event->key() == Qt::Key_3;

	//	float x_displacement = -0.25f * left + 0.25f * right;
	//	float y_displacement = -0.25f * up + 0.25f * down;

	//	for (const auto& i : selections) {
	//		i->position.x += x_displacement;
	//		i->position.y += y_displacement;
	//		i->update();
	//	}

	//	map->doodads.update_doodad_pathing(selections);
	//}

	//if (event->modifiers() & Qt::ControlModifier) {
	//	switch (event->key()) {
	//		case Qt::Key_A:
	//			selections = map->doodads.query_area({ 0.0, 0.0, static_cast<double>(map->terrain.width), static_cast<double>(map->terrain.height) });
	//			break;
	//		case Qt::Key_PageUp:
	//			for (const auto& i : selections) {
	//				i->position.z += 0.1f;
	//				i->update();
	//			}
	//			break;
	//		case Qt::Key_PageDown:
	//			for (const auto& i : selections) {
	//				i->position.z -= 0.1f;
	//				i->update();
	//			}
	//			break;
	//		default:
	//			Brush::key_press_event(event);
	//	}
	//} else {
	//	switch (event->key()) {
	//		case Qt::Key_PageUp:
	//			for (const auto& i : selections) {
	//				i->scale.z += 0.1f;
	//				i->update();
	//			}
	//			break;
	//		case Qt::Key_PageDown:
	//			for (const auto& i : selections) {
	//				i->scale.z -= 0.1f;
	//				i->update();
	//			}
	//			break;
	//		default:
	//			Brush::key_press_event(event);
	//	}
	//}
}

void UnitBrush::key_release_event(QKeyEvent* event) {
	//if (!event->isAutoRepeat()) {
	//	if (doodad_state_undo) {
	//		for (const auto& i : selections) {
	//			doodad_state_undo->new_doodads.push_back(*i);
	//		}
	//		map->terrain_undo.add_undo_action(std::move(doodad_state_undo));
	//	}
	//	//doodad_state_undo
	//}
}

void UnitBrush::mouse_release_event(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && mode == Mode::selection) {
		selection_started = false;
	} else {
		Brush::mouse_release_event(event);
	}
}

void UnitBrush::mouse_move_event(QMouseEvent* event) {
	Brush::mouse_move_event(event);
}

void UnitBrush::delete_selection() {
	//if (selections.size()) {
	//	QRectF update_pathing_area;
	//	// Undo/redo
	//	map->terrain_undo.new_undo_group();
	//	auto action = std::make_unique<DoodadDeleteAction>();
	//	for (const auto& i : selections) {
	//		action->doodads.push_back(*i);

	//		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
	//			update_pathing_area = { i->position.x, i->position.y, 1.f, 1.f };
	//		}
	//		update_pathing_area |= { i->position.x, i->position.y, 1.f, 1.f };
	//	}
	//	map->terrain_undo.add_undo_action(std::move(action));
	//	map->doodads.remove_doodads(selections);

	//	map->doodads.update_doodad_pathing(update_pathing_area);

	//	selections.clear();
	//}
}

void UnitBrush::copy_selection() {
	//clipboard.clear();

	//// Mouse position is average location
	//clipboard_free_placement = true;
	//glm::vec3 average_position = {};
	//for (const auto& i : selections) {
	//	if (i->pathing) {
	//		clipboard_free_placement = false;
	//	}
	//	clipboard.push_back(*i);
	//	average_position += i->position;
	//}
	//clipboard_mouse_position = average_position / static_cast<float>(clipboard.size());
}

void UnitBrush::cut_selection() {
	copy_selection();
	// Delete selection will add to the undo/redo tree
	delete_selection();
}

void UnitBrush::clear_selection() {
	//selections.clear();
}

void UnitBrush::place_clipboard() {
	//apply_begin();
	//for (const auto& i : clipboard) {
	//	Doodad& new_doodad = map->doodads.add_doodad(i);

	//	glm::vec3 final_position;
	//	if (clipboard_free_placement) {
	//		final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);
	//	} else {
	//		final_position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f - glm::vec2(glm::ivec2(clipboard_mouse_position * 4.f)) / 4.f, 0) + i.position;
	//	}
	//	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y);

	//	new_doodad.position = final_position;
	//	new_doodad.update();
	//	doodad_undo->doodads.push_back(new_doodad);

	//	if (new_doodad.pathing) {
	//		map->pathing_map.blit_pathing_texture(new_doodad.position, glm::degrees(rotation) + 90, new_doodad.pathing);
	//	}
	//}
	//map->pathing_map.upload_dynamic_pathing();
	//apply_end();
}

void UnitBrush::apply_begin() {
	//map->terrain_undo.new_undo_group();
	//doodad_undo = std::make_unique<DoodadAddAction>();
}

void UnitBrush::apply() {
	//if (id == "") {
	//	return;
	//}

	//glm::vec3 position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f, input_handler.mouse_world.z);

	////doodad_undo->doodads.push_back(doodad);

	//if (random_rotation) {
	//	set_random_rotation();
	//	set_shape(shape);
	//}
}

void UnitBrush::apply_end() {
//	map->terrain_undo.add_undo_action(std::move(doodad_undo));
}

void UnitBrush::render_brush() const {
	glm::mat4 matrix(1.f);
	matrix = glm::translate(matrix, input_handler.mouse_world);
	matrix = glm::scale(matrix, glm::vec3(1.f / 128.f));
	matrix = glm::rotate(matrix, rotation, glm::vec3(0, 0, 1));

	if (mesh) {
		mesh->render_queue(matrix);
	}
}

void UnitBrush::render_selection() const {
	/*gl->glDisable(GL_DEPTH_TEST);
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
	gl->glEnable(GL_DEPTH_TEST);*/
}

void UnitBrush::render_clipboard() const {
	//for (const auto& i : clipboard) {
	//	glm::vec3 base_scale = glm::vec3(1.f);

	//	if (doodads_slk.row_header_exists(i.id)) {
	//		base_scale = glm::vec3(doodads_slk.data<float>("defScale", i.id));
	//	}

	//	glm::vec3 final_position;
	//	if (clipboard_free_placement) {
	//		final_position = glm::vec3(glm::vec2(input_handler.mouse_world + i.position) - clipboard_mouse_position, 0);
	//	} else {
	//		// clipboard_mouse_position only per 0.5 units?
	//		final_position = glm::vec3(glm::vec2(position) + glm::vec2(uv_offset) * 0.25f + size * 0.125f - glm::vec2(glm::ivec2(clipboard_mouse_position * 2.f)) / 2.f, 0) + i.position;
	//	}
	//	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y);

	//	glm::mat4 model(1.f);
	//	model = glm::translate(model, final_position);
	//	model = glm::scale(model, (base_scale - 1.f + i.scale) / 128.f);
	//	model = glm::rotate(model, i.angle, glm::vec3(0, 0, 1));

	//	i.mesh->render_queue(model);
	//}
}

void UnitBrush::set_random_rotation() {
	//std::random_device rd;
	//std::mt19937 gen(rd());

	//bool fixed_rotation = false;
	//if (doodads_slk.row_header_exists(id)) {
	//	fixed_rotation = doodads_slk.data<int>("fixedRot", id) > 0;
	//} else {
	//	fixed_rotation = destructibles_slk.data<int>("fixedRot", id) > 0;
	//}

	//std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
	//float target_rotation = dist(gen);
	//if (pathing_texture && pathing_texture->width == pathing_texture->height) {
	//	if (pathing_texture->homogeneous) {
	//		rotation = target_rotation;
	//	} else {
	//		rotation = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4)* glm::pi<float>() * 0.5f;
	//	}
	//} else {
	//	rotation = (static_cast<int>((target_rotation + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4)* glm::pi<float>() * 0.5f;
	//}
}

void UnitBrush::set_unit(const std::string& id) {
	this->id = id;
	mesh = map->units.get_mesh(id);
}