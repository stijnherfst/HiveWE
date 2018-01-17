#include "stdafx.h"

void FPSCamera::update(double delta) {
	float speed = 5;
	if (input_handler.key_pressed(Qt::Key_Shift)) {
		speed = 20;
	}

	if (input_handler.key_pressed(Qt::Key_W)) {
		position += direction * speed * (float)delta;
	} else if (input_handler.key_pressed(Qt::Key_S)) {
		position -= direction * speed * (float)delta;
	}

	if (input_handler.key_pressed(Qt::Key_A)) {
		position -= glm::normalize(glm::cross(direction, up)) * speed * (float)delta;
	} else if (input_handler.key_pressed(Qt::Key_D)) {
		position += glm::normalize(glm::cross(direction, up)) * speed * (float)delta;
	}

	if (input_handler.key_pressed(Qt::Key_Space)) {
		position.z += 1 * speed * (float)delta;
	} else if (input_handler.key_pressed(Qt::Key_Control)) {
		position.z -= 1 * speed * (float)delta;
	}
	
	int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
	int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();

	horizontalAngle += diffx * 0.1 * speed * (float)delta;
	verticalAngle += diffy * 0.1 * speed * (float)delta;

	verticalAngle = std::max(-89.0, std::min(verticalAngle, 89.0));

	direction = glm::vec3(
		std::cos(verticalAngle) * std::sin(horizontalAngle),
		std::cos(verticalAngle) * std::cos(horizontalAngle),
		std::sin(verticalAngle)
	);

	direction = glm::normalize(direction);

	projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
	view = glm::lookAt(position, position + direction, up);
	projection_view = projection * view;
}

void FPSCamera::mouse_move_event(QMouseEvent* event) {

}


void TPSCamera::update(double delta) {

};

void TPSCamera::mouse_move_event(QMouseEvent* event) {
	if (event->buttons() == Qt::RightButton) {
		int diffx = input_handler.mouse.x() - input_handler.previous_mouse.x();
		int diffy = input_handler.mouse.y() - input_handler.previous_mouse.y();
		input_handler.previous_mouse = event->globalPos();

		position += glm::vec3(-diffx * 0.025, diffy * 0.025, 0);

		verticalAngle = 1;
		horizontalAngle = 3.1415;

		direction = glm::vec3(
			std::cos(verticalAngle) * std::sin(horizontalAngle),
			std::cos(verticalAngle) * std::cos(horizontalAngle),
			std::sin(verticalAngle)
		);

		direction = glm::normalize(direction);

		projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
		view = glm::lookAt(position + direction * distance, position, up);
		projection_view = projection * view;
	}
}

TPSCamera camera;