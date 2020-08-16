#include "Camera.h"

#include "HiveWE.h"
#include "InputHandler.h"

void Camera::reset() {
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

	// for billboarded animated mesh
	glm::vec3 opDirection = -direction; // camera->position - this->position;

	glm::vec3 opDirectionZ = glm::normalize(glm::vec3(opDirection.x, opDirection.y, 0));
	float angleZ = glm::atan(opDirectionZ.y, opDirectionZ.x);
	glm::vec3 axisZ = glm::vec3(0, 0, 1);

	glm::vec3 opDirectionY = glm::normalize(opDirection);
	glm::vec3 axisY = glm::vec3(0, -1, 0);

	decomposed_rotation = glm::angleAxis(angleZ, axisZ) * glm::angleAxis(glm::asin(opDirectionY.z), axisY);
}

void FPSCamera::mouse_move_event(QMouseEvent* event) {
	glm::vec2 diff = input_handler.mouse - input_handler.previous_mouse;

	horizontal_angle += diff.x * 0.012;
	vertical_angle += -diff.y * 0.012;
	vertical_angle = std::max(-glm::pi<double>() / 2 + 0.001, std::min(vertical_angle, glm::pi<double>() / 2 - 0.001));

	update(0);
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

	// for billboarded animated mesh
	glm::vec3 opDirection = -direction; // camera->position - this->position;

	glm::vec3 opDirectionZ = glm::normalize(glm::vec3(opDirection.x, opDirection.y, 0));
	float angleZ = glm::atan(opDirectionZ.y, opDirectionZ.x);
	glm::vec3 axisZ = glm::vec3(0, 0, 1);

	glm::vec3 opDirectionY = glm::normalize(opDirection);
	glm::vec3 axisY = glm::vec3(0, -1, 0);

	decomposed_rotation = glm::angleAxis(angleZ, axisZ) * glm::angleAxis(glm::asin(opDirectionY.z), axisY);
}

void TPSCamera::mouse_move_event(QMouseEvent* event) {
	glm::vec2 diff = input_handler.mouse - input_handler.previous_mouse;

	if (rolling || (event->buttons() == Qt::RightButton && event->modifiers() & Qt::ControlModifier)) {
		horizontal_angle += -diff.x * 0.0025f;
		vertical_angle += diff.y * 0.0025f;
		vertical_angle = std::max(-glm::pi<double>() / 2 + 0.001, std::min(vertical_angle, glm::pi<double>() / 2 - 0.001));
		update(0);
	} else if (event->buttons() == Qt::RightButton) {
		position += X * (-diff.x * 0.025f * (distance / 30.f));
		position += forward * (-diff.y * 0.025f * (distance / 30.f));
		position.z = map->terrain.interpolated_height(position.x, position.y);
		update(0);
	}
}

void TPSCamera::mouse_scroll_event(QWheelEvent* event) {
	distance = std::clamp(distance * std::pow(0.999f, static_cast<float>(event->angleDelta().y())), 0.001f, 1000.f);
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