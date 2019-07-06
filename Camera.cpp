#include "stdafx.h"

void Camera::reset() {
	position = glm::vec3(map->terrain.width / 2, map->terrain.height / 2, 0);
	position.z = map->terrain.interpolated_height(position.x, position.y);

	distance = 20;
	horizontal_angle = 0.0;
	vertical_angle = -0.977;
	update(0);
}

void FPSCamera::update(const double delta) {
	float speed = 5;
	if (input_handler.key_pressed(Qt::Key_Shift)) {
		speed = 20;
	}

	if (input_handler.key_pressed(Qt::Key_W)) {
		position += direction * speed * float(delta);
	} else if (input_handler.key_pressed(Qt::Key_S)) {
		position -= direction * speed * float(delta);
	}

	const glm::vec3 displacement = glm::normalize(glm::cross(direction, up)) * speed * float(delta);
	if (input_handler.key_pressed(Qt::Key_A)) {
		position -= displacement;
	} else if (input_handler.key_pressed(Qt::Key_D)) {
		position += displacement;
	}

	if (input_handler.key_pressed(Qt::Key_Space)) {
		position.z += 1 * speed * float(delta);
	} else if (input_handler.key_pressed(Qt::Key_Control)) {
		position.z -= 1 * speed * float(delta);
	}

	direction = glm::vec3(
		std::cos(vertical_angle) * std::sin(horizontal_angle),
		std::cos(vertical_angle) * std::cos(horizontal_angle),
		std::sin(vertical_angle)
	);

	direction = glm::normalize(direction);

	projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance);
	view = glm::lookAt(position, position + direction, up);
	projection_view = projection * view;
}

void FPSCamera::mouse_move_event(QMouseEvent* event) {
	const int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
	const int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();

	horizontal_angle += diffx * 0.012;
	vertical_angle += -diffy * 0.012;
	vertical_angle = std::max(-glm::pi<double>() / 2 + 0.001, std::min(vertical_angle, glm::pi<double>() / 2 - 0.001));

	update(0);

	input_handler.previous_mouse = event->globalPos();
}

void FPSCamera::mouse_scroll_event(QWheelEvent* event) {
}


void TPSCamera::update(double delta) {
	direction = glm::vec3(
		std::cos(vertical_angle) * std::sin(horizontal_angle),
		std::cos(vertical_angle) * std::cos(horizontal_angle),
		std::sin(vertical_angle)
	);

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

	if (input_handler.key_pressed(Qt::Key_Left)) { //glm::vec3(-40, 0, 0)
		position +=  -X * 40.f * static_cast<float>(delta) * (distance / 30.f);
	} else if (input_handler.key_pressed(Qt::Key_Right)) {
		position += X * 40.f * static_cast<float>(delta) * (distance / 30.f);
	}

	if (input_handler.key_pressed(Qt::Key_Up)) {
		position += -forward * 40.f * static_cast<float>(delta) * (distance / 30.f);
	} else if (input_handler.key_pressed(Qt::Key_Down)) {
		position += forward * 40.f * static_cast<float>(delta) * (distance / 30.f);
	}
	position.z = map->terrain.interpolated_height(position.x, position.y);

	projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance);
	view = glm::lookAt(position - direction * distance, position, up);
	projection_view = projection * view;
}

void TPSCamera::mouse_move_event(QMouseEvent* event) {
	const int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
	const int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();

	if (rolling || (event->buttons() == Qt::RightButton && event->modifiers() & Qt::ControlModifier)) {
		horizontal_angle += -diffx * 0.0025f;
		vertical_angle += diffy * 0.0025f;
		vertical_angle = std::max(-glm::pi<double>() / 2 + 0.001, std::min(vertical_angle, glm::pi<double>() / 2 - 0.001));
		update(0);
	} else if (event->buttons() == Qt::RightButton) {
		position += X * (-diffx * 0.025f * (distance / 30.f));
		position += forward * (-diffy * 0.025f * (distance / 30.f));
		position.z = map->terrain.interpolated_height(position.x, position.y);
		update(0);
	}

	input_handler.previous_mouse = event->globalPos();
}

void TPSCamera::mouse_scroll_event(QWheelEvent* event) {
	distance = std::clamp(distance * std::pow(0.999f, event->angleDelta().y()), 0.001f, 1000.f);
	update(0);
}

void TPSCamera::mouse_press_event(QMouseEvent* event) {
	switch (event->button()) {
		case Qt::MiddleButton:
			rolling = true;
        default:
			break;
	}
}

void TPSCamera::mouse_release_event(QMouseEvent* event) {
	switch (event->button()) {
		case Qt::MiddleButton:
			rolling = false;
        default:
			break;
	}
}

Camera* camera;