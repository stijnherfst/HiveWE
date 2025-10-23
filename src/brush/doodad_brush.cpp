#include "doodad_brush.h"

#include <QKeyEvent>

import std;
import Hierarchy;
import SLK;
import Texture;
import WorldUndoManager;
import Camera;
import OpenGLUtilities;
import ResourceManager;
import PathingMap;
import SkinnedMesh;
import SkeletalModelInstance;
import Globals;
import MapGlobal;
import <glad/glad.h>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

DoodadBrush::DoodadBrush()
	: Brush() {
	granularity = 2.f;
	uv_offset_granularity = 2;
	brush_offset = { 0.0f, 0.0f };

	click_helper = resource_manager.load<SkinnedMesh>("Objects/InvalidObject/InvalidObject.mdx", "", std::nullopt);
	click_helper_skeleton = SkeletalModelInstance(click_helper->model);
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
	context->makeCurrent();
	shape = new_shape;

	glDeleteTextures(1, &brush_texture);
	glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (doodad.pathing) {
		// 270 instead of 90 because OpenGL upside down shenanigans
		const int32_t rotation = ((int)glm::degrees(doodad.angle) + 270);

		const int div_w = (rotation % 180) ? doodad.pathing->height : doodad.pathing->width;
		const int div_h = (rotation % 180) ? doodad.pathing->width : doodad.pathing->height;
		
		std::vector<glm::u8vec4> brush(div_w * div_h, { 0, 0, 0, 0 });

		for (int i = 0; i < doodad.pathing->width; i++) {
			for (int j = 0; j < doodad.pathing->height; j++) {
				int x = i;
				int y = j;

				switch (rotation % 360) {
					case 90:
						x = doodad.pathing->height - 1 - j;
						y = i;
						break;
					case 180:
						x = doodad.pathing->width - 1 - i;
						y = doodad.pathing->height - 1 - j;
						break;
					case 270:
						x = j;
						y = doodad.pathing->width - 1 - i;
						break;
				}

				const int in = ((doodad.pathing->height - 1 - j) * doodad.pathing->width + i) * doodad.pathing->channels;

				// Have to check for > 250 because sometimes the pathing textures are not properly thresholded
				glm::u8vec4 color = { 
					doodad.pathing->data[in] > 250 ? 255 : 0,
					doodad.pathing->data[in + 1] > 250 ? 255 : 0,
					doodad.pathing->data[in + 2] > 250 ? 255 : 0,
					128 
				};

				if (color.r || color.g || color.b) {
					brush[y * div_w + x] = color;
				}
			}
		}
		glTextureStorage2D(brush_texture, 1, GL_RGBA8, div_w, div_h);
		glTextureSubImage2D(brush_texture, 0, 0, 0, div_w, div_h, GL_RGBA, GL_UNSIGNED_BYTE, brush.data());
	} else {
		std::vector<glm::u8vec4> brush(size * size, { 0, 0, 0, 0 });
		glTextureStorage2D(brush_texture, 1, GL_RGBA8, size, size);
		glTextureSubImage2D(brush_texture, 0, 0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, brush.data());
	}
}

void DoodadBrush::key_press_event(QKeyEvent* event) {
	if (event->modifiers() & Qt::KeypadModifier) {
		if (action == Action::none) {
			start_action(Action::move);
		}

		bool left = event->key() == Qt::Key_1 || event->key() == Qt::Key_4 || event->key() == Qt::Key_7;
		bool right = event->key() == Qt::Key_3 || event->key() == Qt::Key_6 || event->key() == Qt::Key_9;
		bool down = event->key() == Qt::Key_7 || event->key() == Qt::Key_8 || event->key() == Qt::Key_9;
		bool up = event->key() == Qt::Key_1 || event->key() == Qt::Key_2 || event->key() == Qt::Key_3;

		bool free_movement = true;
		for (const auto& i : selections) {
			free_movement = free_movement && !i->pathing;
		}

		float x_displacement;
		float y_displacement;
		if (free_movement) {
			x_displacement = -0.25f * left + 0.25f * right;
			y_displacement = -0.25f * up + 0.25f * down;
		} else {
			x_displacement = -0.5f * left + 0.5f * right;
			y_displacement = -0.5f * up + 0.5f * down;
		}

		for (const auto& i : selections) {
			i->position.x += x_displacement;
			i->position.y += y_displacement;
			if (!lock_doodad_z) {
				i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y, true);
			}
			i->update(map->terrain);
		}
		emit position_changed();

		map->doodads.update_doodad_pathing(selections, map->pathing_map);
	}

	if (event->modifiers() & Qt::ControlModifier) {
		switch (event->key()) {
			case Qt::Key_A:
				selections.clear();
				selections.reserve(map->doodads.doodads.size());
				for (auto& i : map->doodads.doodads) {
					selections.emplace(&i);
				}

				emit selection_changed();
				break;
			case Qt::Key_PageUp:
				if (action == Action::none) {
					start_action(Action::move);
				}
				for (const auto& i : selections) {
					i->position.z += 0.1f;
					i->update(map->terrain);
				}
				emit position_changed();
				break;
			case Qt::Key_PageDown:
				if (action == Action::none) {
					start_action(Action::move);
				}
				for (const auto& i : selections) {
					i->position.z -= 0.1f;
					i->update(map->terrain);
				}
				emit position_changed();
				break;
			default:
				Brush::key_press_event(event);
		}
	} else {
		switch (event->key()) {
			case Qt::Key_PageUp:
				if (action == Action::none) {
					start_action(Action::move);
				}
				for (const auto& i : selections) {
					i->scale.z += 0.1f;
					i->update(map->terrain);
				}
				emit scale_changed();
				break;
			case Qt::Key_PageDown:
				if (action == Action::none) {
					start_action(Action::move);
				}
				for (const auto& i : selections) {
					i->scale.z -= 0.1f;
					i->update(map->terrain);
				}
				emit scale_changed();
				break;
			default:
				Brush::key_press_event(event);
		}
	}
}

void DoodadBrush::key_release_event(QKeyEvent* event) {
	if (event->isAutoRepeat()) {
		return;
	}

	if (action == Action::move) {
		end_action();
	}
}

void DoodadBrush::mouse_press_event(QMouseEvent* event, double frame_delta) {
	// The mouse.y check is needed as sometimes it is negative for unknown reasons
	if (event->button() == Qt::LeftButton && input_handler.mouse.y > 0.f) {
		if (mode == Mode::selection) {
			if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier) {
				auto id = map->render_manager.pick_doodad_id_under_mouse(map->doodads, input_handler.mouse);
				if (id) {
					if (selections.contains(&map->doodads.doodads[id.value()])) {
						selections.erase(&map->doodads.doodads[id.value()]);
					} else {
						selections.emplace(&map->doodads.doodads[id.value()]);
					}
					return;
				}
			}

			if (!event->modifiers()) {
				auto id = map->render_manager.pick_doodad_id_under_mouse(map->doodads, input_handler.mouse);
				if (id) {
					Doodad& doodad = map->doodads.doodads[id.value()];

					drag_start = input_handler.mouse_world;
					dragging = true;

					// If the current index is already in a selection then we want to drag the entire group
					if (std::ranges::find(selections, &doodad) != selections.end()) {
						drag_offsets.clear();
						for (const auto& i : selections) {
							drag_offsets.push_back(input_handler.mouse_world - i->position);
						}
					} else {
						selections = { &doodad };
						drag_offsets = { input_handler.mouse_world - doodad.position };
						emit selection_changed();
					}
					return;
				}
			}
		}
	}
	if (event->button() == Qt::MiddleButton && input_handler.mouse.y > 0.f) {
		auto id = map->render_manager.pick_doodad_id_under_mouse(map->doodads, input_handler.mouse);

		if (id) {
			Doodad& doodad = map->doodads.doodads[id.value()];
			emit request_doodad_select(doodad.id);
		}
	}
	Brush::mouse_press_event(event, frame_delta);
}

void DoodadBrush::mouse_move_event(QMouseEvent* event, double frame_delta) {
	Brush::mouse_move_event(event, frame_delta);

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::selection) {
			if (dragging) {
				if (action == Action::none) {
					start_action(Action::drag);
				}

				bool free_movement = true;
				for (const auto& i : selections) {
					free_movement = free_movement && !i->pathing;
				}

				glm::vec3 offset;
				if (free_movement) {
					offset = input_handler.mouse_world - drag_start;
				} else {
					offset = glm::round((input_handler.mouse_world) * 2.f + 0.5f) / 2.f - 0.25f;
					offset -= glm::round((drag_start)*2.f + 0.5f) / 2.f - 0.25f;
				}
				offset.z = 0;

				if (!free_movement && offset.x == 0.f && offset.y == 0.f) {
					return;
				}
				drag_start = input_handler.mouse_world;

				for (const auto& doodad : selections) {
					doodad->position += offset;
					if (!lock_doodad_z) {
						doodad->position.z = map->terrain.interpolated_height(doodad->position.x, doodad->position.y, true);
					}
					doodad->update(map->terrain);
				}
				emit position_changed();
				map->doodads.update_doodad_pathing(selections, map->pathing_map);
			} else if (event->modifiers() & Qt::ControlModifier) {
				if (action == Action::none) {
					start_action(Action::rotate);
				}

				for (auto&& i : selections) {
					float target_rotation = std::atan2(input_handler.mouse_world.y - i->position.y, input_handler.mouse_world.x - i->position.x);
					if (target_rotation < 0) {
						target_rotation += 2.f * glm::pi<float>();
					}

					i->angle = Doodad::acceptable_angle(i->id, i->pathing, i->angle, target_rotation);
					i->position = glm::vec3(Doodad::acceptable_position(i->position, i->pathing, i->angle), i->position.z);
					i->update(map->terrain);
				}
				emit angle_changed();

				map->doodads.update_doodad_pathing(selections, map->pathing_map);
			} else if (mode == Mode::selection && selection_started) {
				const glm::vec3 size = input_handler.mouse_world - selection_start;

				auto query = map->doodads.query_area({ selection_start.x, selection_start.y, size.x, size.y });
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

				emit selection_changed();
			}
		}
	}
}

void DoodadBrush::mouse_release_event(QMouseEvent* event) {
	dragging = false;

	if (event->button() == Qt::LeftButton) {
		if (action == Action::drag || action == Action::rotate) {
			end_action();
		}
	}

	Brush::mouse_release_event(event);
}

void DoodadBrush::delete_selection() {
	if (!selections.size()) {
		return;
	}

	QRectF update_pathing_area;
	// Undo/redo
	auto action = std::make_unique<DoodadDeleteAction>();
	for (const auto& i : selections) {
		action->doodads.push_back(*i);

		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i->position.x, i->position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i->position.x, i->position.y, 1.f, 1.f };
	}
	map->world_undo.new_undo_group();
	map->world_undo.add_undo_action(std::move(action));

	map->doodads.remove_doodads(selections);
	map->doodads.update_doodad_pathing(update_pathing_area, map->pathing_map);

	selections.clear();
	emit selection_changed();
}

void DoodadBrush::copy_selection() {
	clipboard.clear();

	// Mouse position is average location
	clipboard_force_grid_aligned = false;
	glm::vec3 average_position = {};
	for (const auto& i : selections) {
		if (i->pathing) {
			clipboard_force_grid_aligned = true;
		}
		clipboard.push_back(*i);
		average_position += i->position;
	}
	clipboard_mouse_offset = average_position / static_cast<float>(clipboard.size());
}

void DoodadBrush::cut_selection() {
	copy_selection();
	// Delete selection will add to the undo/redo tree
	delete_selection();
}

void DoodadBrush::clear_selection() {
	selections.clear();
	emit selection_changed();
}

void DoodadBrush::place_clipboard() {
	apply_begin();
	for (const auto& i : clipboard) {
		Doodad& new_doodad = map->doodads.add_doodad(i);
		new_doodad.creation_number = ++Doodad::auto_increment;

		glm::vec3 final_position = glm::vec3(Doodad::acceptable_position(glm::vec2(input_handler.mouse_world) + glm::vec2(i.position) - clipboard_mouse_offset, i.pathing, i.angle), i.position.z);
		if (!lock_doodad_z) {
			final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true);
		}

		new_doodad.position = final_position;
		new_doodad.update(map->terrain);
		doodad_undo->doodads.push_back(new_doodad);

		if (new_doodad.pathing) {
			map->pathing_map.blit_pathing_texture(new_doodad.position, glm::degrees(new_doodad.angle) + 90, new_doodad.pathing);
		}
	}
	map->pathing_map.upload_dynamic_pathing();
	apply_end();
}

void DoodadBrush::apply_begin() {
	doodad_undo = std::make_unique<DoodadAddAction>();
}

void DoodadBrush::apply(double frame_delta) {
	if (doodad.id == "") {
		return;
	}

	doodad.creation_number = ++Doodad::auto_increment;
	map->doodads.add_doodad(doodad);

	doodad_undo->doodads.push_back(doodad);

	if (doodad.pathing) {
		map->pathing_map.blit_pathing_texture(doodad.position, glm::degrees(doodad.angle) + 90, doodad.pathing);
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

		const bool is_doodad = doodads_slk.row_headers.contains(doodad.id);
		const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

		float min_scale = slk.data<float>("minscale", doodad.id);
		float max_scale = slk.data<float>("maxscale", doodad.id);
		std::uniform_real_distribution dist(min_scale, max_scale);
		doodad.scale = glm::vec3(dist(gen));
	}
}

void DoodadBrush::apply_end() {
	if (doodad_undo->doodads.empty()) {
		return;
	}
	map->world_undo.new_undo_group();
	map->world_undo.add_undo_action(std::move(doodad_undo));
}

void DoodadBrush::render_brush() {
	if (doodad.pathing) {
		Brush::render_brush();
	}

	if (!doodad.mesh) {
		return;
	}

	glm::vec3 final_position = glm::vec3(Doodad::acceptable_position(input_handler.mouse_world, doodad.pathing, doodad.angle), 0.f);
	final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, false);

	doodad.position = final_position;
	doodad.update(map->terrain);
	doodad.skeleton.update(0.016f);
	map->render_manager.queue_render(*doodad.mesh, doodad.skeleton, doodad.color, 0);

	const bool is_doodad = doodads_slk.row_headers.contains(doodad.id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
	const bool use_click_helper = slk.data<bool>("useclickhelper", doodad.id);
	if (use_click_helper) {
		click_helper_skeleton.matrix = doodad.skeleton.matrix;
		click_helper_skeleton.update(0.016f);
		map->render_manager.queue_render(*click_helper, click_helper_skeleton, glm::vec3(1.f), 0);
	}
}

// Quads are drawn and then in the fragment shader fragments are discarded to form a circle
void DoodadBrush::render_selection() const {
	glDisable(GL_DEPTH_TEST);
	selection_circle_shader->use();
	glEnableVertexAttribArray(0);

	for (const auto& i : selections) {
		float selection_scale = 1.f;
		if (i->mesh->model->sequences.empty()) {
			selection_scale = i->mesh->model->extent.bounds_radius / 128.f;
		} else {
			selection_scale = i->mesh->model->sequences[i->skeleton.sequence_index].extent.bounds_radius / 128.f;
		}
		
		bool is_doodad = doodads_slk.row_headers.contains(i->id);
		slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
		bool use_click_helper = slk.data<bool>("useclickhelper", i->id);

		if (use_click_helper) {
			selection_scale = std::max(selection_scale, click_helper->model->extent.bounds_radius / 128.f);
		}
		if (selection_scale < 0.1f) { // Todo hack, what is the correct approach?
			selection_scale = i->mesh->model->extent.bounds_radius / 128.f;
		}

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

void DoodadBrush::render_clipboard() {
	for (auto& i : clipboard) {
		glm::vec3 final_position = glm::vec3(Doodad::acceptable_position(glm::vec2(input_handler.mouse_world) + glm::vec2(i.position) - clipboard_mouse_offset, i.pathing, i.angle, clipboard_force_grid_aligned), i.position.z);

		if (!lock_doodad_z) {
			final_position.z = map->terrain.interpolated_height(final_position.x, final_position.y, true);
		}

		const auto previous_position = i.position;
		i.position = final_position;
		i.update(map->terrain);
		i.skeleton.update(0.016f);
		i.position = previous_position;

		map->render_manager.queue_render(*i.mesh, i.skeleton, glm::vec3(1.f), 0);
	}
}

bool DoodadBrush::can_place() {
	if (!doodad.pathing) {
		return true;
	}

	return map->pathing_map.is_area_free(doodad.position, glm::degrees(doodad.angle) + 90, doodad.pathing, PathingMap::Flags::unwalkable | PathingMap::Flags::unflyable | PathingMap::Flags::unbuildable);
}

void DoodadBrush::set_random_variation() {
	variation = get_random_variation();
	context->makeCurrent();
	doodad.init(doodad.id, map->doodads.get_mesh(doodad.id, variation), map->terrain);
}

void DoodadBrush::set_random_rotation() {
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution dist(0.f, glm::pi<float>() * 2.f);
	float target_rotation = dist(gen);
	doodad.angle = Doodad::acceptable_angle(doodad.id, doodad.pathing, target_rotation, target_rotation);

	if (doodad.pathing) {
		auto rotated_pathing_width = doodad.pathing->width;
		auto rotated_pathing_height = doodad.pathing->height;

		if (static_cast<uint32_t>(glm::round(glm::degrees(doodad.angle))) % 180 == 0) {
			rotated_pathing_width = doodad.pathing->height;
			rotated_pathing_height = doodad.pathing->width;
		}

		brush_offset.x = (rotated_pathing_width % 4 != 0) ? 0.25f : 0.f;
		brush_offset.y = (rotated_pathing_height % 4 != 0) ? 0.25f : 0.f;
	}

	doodad.update(map->terrain);
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
	context->makeCurrent();

	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	possible_variations.clear();
	int variation_count = slk.data<int>("numvar", id);
	for (int i = 0; i < variation_count; i++) {
		possible_variations.insert(i);
	}
	variation = get_random_variation();

	doodad.init(id, map->doodads.get_mesh(id, variation), map->terrain);

	// It might be initially incorrect because another doodad.angle is not reset in init()
	doodad.angle = Doodad::acceptable_angle(doodad.id, doodad.pathing, doodad.angle, doodad.angle);
	if (random_rotation) {
		set_random_rotation();
	}

	// Same as above for the scale
	float min_scale = slk.data<float>("minscale", doodad.id);
	float max_scale = slk.data<float>("maxscale", doodad.id);
	doodad.scale = glm::clamp(doodad.scale, min_scale, max_scale);
	if (random_scale) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution dist(min_scale, max_scale);
		doodad.scale = glm::vec3(dist(gen));
	}

	if (doodad.pathing) {
		set_size(std::max(doodad.pathing->width, doodad.pathing->height));
	}
	set_shape(shape);
}

void DoodadBrush::start_action(Action new_action) {
	action = new_action;
	map->world_undo.new_undo_group();
	doodad_state_undo = std::make_unique<DoodadStateAction>();
	for (const auto& i : selections) {
		doodad_state_undo->old_doodads.push_back(*i);
	}
}

void DoodadBrush::end_action() {
	for (const auto& i : selections) {
		doodad_state_undo->new_doodads.push_back(*i);
	}
	map->world_undo.add_undo_action(std::move(doodad_state_undo));
	action = Action::none;
}

void DoodadBrush::set_selection_angle(float angle) {
	start_action(Action::rotate);
	for (auto& i : selections) {
		i->angle = Doodad::acceptable_angle(i->id, i->pathing, i->angle, angle);
		i->position = glm::vec3(Doodad::acceptable_position(i->position, i->pathing, i->angle), i->position.z);
		i->update(map->terrain);
	}
	map->doodads.update_doodad_pathing(selections, map->pathing_map);
	end_action();
}

void DoodadBrush::set_selection_absolute_height(float height) {
	start_action(Action::move);
	for (auto& i : selections) {
		i->position.z = height;
		i->update(map->terrain);
	}
	end_action();
}

void DoodadBrush::set_selection_relative_height(float height) {
	start_action(Action::move);
	for (auto& i : selections) {
		i->position.z = map->terrain.interpolated_height(i->position.x, i->position.y, true) + height;
		i->update(map->terrain);
	}
	end_action();
}

void DoodadBrush::set_selection_scale_component(int component, float scale) {
	start_action(Action::scale);
	for (auto& i : selections) {
		bool is_doodad = doodads_slk.row_headers.contains(i->id);
		slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

		float min_scale = slk.data<float>("minscale", i->id);
		float max_scale = slk.data<float>("maxscale", i->id);

		if (!is_doodad) {
			i->scale = glm::vec3(std::clamp(scale, min_scale, max_scale));
		} else {
			i->scale[component] = std::clamp(scale, min_scale, max_scale);
		}
		i->update(map->terrain);
	}
	end_action();
}

void DoodadBrush::unselect_id(std::string_view id) {
	if (doodad.id == id) {
		set_doodad("ATtr");
	}
}