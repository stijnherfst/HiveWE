#include "model_editor_camera.h"

void ModelEditorCamera::reset() {
	distance = 20;
	horizontal_angle = 0.0f;
	vertical_angle = -0.977f;
	update(0);
}

void ModelEditorCamera::update(double delta) {
	direction = glm::vec3(
		std::cos(vertical_angle) * std::sin(horizontal_angle),
		std::cos(vertical_angle) * std::cos(horizontal_angle),
		std::sin(vertical_angle));

	direction = glm::normalize(direction);
	// Calculate axis directions for camera as referential point:
	// Z axis is simply the direction we are facing
	// X axis is then the cross product between the "fake" up and Z
	X = glm::cross(direction, up);
	X = glm::normalize(X);
	// Y Axis is cross product between X and Z, e.g is the real up
	Y = glm::cross(X, direction);
	Y = glm::normalize(Y);

	// The vector that is perpendicular to the up vector, thus points forward
	forward = glm::cross(X, up);

	projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance_far);
	view = glm::lookAt(position - direction * distance, position, up);
	projection_view = projection * view;

	// for billboarded animated mesh
	glm::vec3 opDirection = -direction; // camera->position - this->position;

	glm::vec3 opDirectionZ = glm::normalize(glm::vec3(opDirection.x, opDirection.y, 0));
	float angleZ = glm::atan(opDirectionZ.y, opDirectionZ.x);
	glm::vec3 axisZ = glm::vec3(0, 0, 1);

	glm::vec3 opDirectionY = glm::normalize(opDirection);
	glm::vec3 axisY = glm::vec3(0, -1, 0);

	decomposed_rotation = glm::angleAxis(angleZ, axisZ) * glm::angleAxis(glm::asin(opDirectionY.z), axisY);
}

void ModelEditorCamera::mouse_move_event(QMouseEvent* event, const InputHandler& my_input_handler) {
	glm::vec2 diff = my_input_handler.mouse - my_input_handler.previous_mouse;

	if (rolling || (event->buttons() == Qt::RightButton && event->modifiers() & Qt::ControlModifier)) {
		horizontal_angle += diff.x * 0.01f;
		vertical_angle -= diff.y * 0.01f;
		vertical_angle = std::max(-glm::pi<float>() / 2.f + 0.001f, std::min(vertical_angle, glm::pi<float>() / 2.f - 0.001f));
		update(0);
	} else if (event->buttons() == Qt::RightButton) {
		position += X * (-diff.x * 0.025f * (distance / 30.f));
		position += forward * (-diff.y * 0.025f * (distance / 30.f));
		update(0);
	}
}

void ModelEditorCamera::mouse_scroll_event(QWheelEvent* event) {
	distance = std::clamp(distance * std::pow(0.999f, static_cast<float>(event->angleDelta().y())), 0.001f, 1000.f);
	update(0);
}

void ModelEditorCamera::mouse_press_event(QMouseEvent* event) {
	switch (event->button()) {
		case Qt::MiddleButton:
			rolling = true;
			break;
		default:
			break;
	}
}

void ModelEditorCamera::mouse_release_event(QMouseEvent* event) {
	switch (event->button()) {
		case Qt::MiddleButton:
			rolling = false;
			break;
		default:
			break;
	}
}