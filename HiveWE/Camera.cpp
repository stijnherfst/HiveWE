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
	distance = std::clamp(distance - event->angleDelta().y() * distance * 0.0008f, 0.05f, 400.f);
	update(0);
}


/*!
	Checks if a point is within the defined camera frustum.
	The point is expected to be in the same coordinate system,
	i.e divide wc3 object positions by 128.0 before passing.
*/
bool TPSCamera::is_visible(glm::vec3 && point)
{
	// Calculate real camera position, together with z-axis
	auto cam_pos = position;
	cam_pos.z = -distance; // Opengl flips z-axis (like a lens)
	auto v = point - cam_pos;
	// First check if point is within frustum in z-axis (if its too far or too close)
	if (v.z > view_distance || v.z < view_distance_close) {
		return false;
	}
	// Some quick trigonometry to calculate height of at point distance
	// half_height / z = tan(fov / 2)
	auto fov_rad = (glm::pi<float>() / 180.f) * static_cast<float>(fov); // Need radians
	auto height = v.z * glm::tan(fov_rad * 0.5f); // Note: this is half_height
	if (v.y > height || v.y + height < 0) {
		return false;
	}

	// For x-axis similarly but width instead
	// use aspect ratio to get correct fov
	fov_rad *= aspect_ratio;
	auto width = v.z * glm::tan(fov_rad * 0.5f);
	if (v.x > width || v.x + width < 0) {
		return false;
	}

	// Point must be in frustum
	return true;
}


TPSCamera camera;