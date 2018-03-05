#include "stdafx.h"

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

	projection = glm::perspective(fov, aspect_ratio, 0.1, 1000.0);
	view = glm::lookAt(position, position + direction, up);
	projection_view = projection * view;
}

void FPSCamera::mouse_move_event(QMouseEvent* event) {
	const int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
	const int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();

	horizontal_angle += diffx * 0.025;
	vertical_angle += diffy * 0.025;
	vertical_angle = std::max(-89.0, std::min(vertical_angle, 89.0));

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

	projection = glm::perspective(fov, aspect_ratio, 0.1, 1000.0);
	view = glm::lookAt(position + direction * distance, position, up);
	projection_view = projection * view;
}

void TPSCamera::mouse_move_event(QMouseEvent* event) {
	if (event->buttons() == Qt::RightButton) {
		const int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
		const int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();

		position += glm::vec3(-diffx * 0.025, diffy * 0.025, 0);

		update(0);
	}
	input_handler.previous_mouse = event->globalPos();
}

void TPSCamera::mouse_scroll_event(QWheelEvent* event) {
	distance = std::max(0.05f, distance - event->angleDelta().y() / 120.f);
	update(0);
}

TPSCamera camera;