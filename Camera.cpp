#include "Camera.h"

#include "HiveWE.h"
#include "InputHandler.h"

void Camera::reset() {
	distance = 20;
	horizontal_angle = 0.0;
	vertical_angle = -0.977;
	update(0);
}

void Camera::extract_frustrum_planes(const glm::mat4& proj, const glm::mat4& view) {
	double clip[4][4];
	clip[0][0] = view[0][0] * proj[0][0] + view[0][1] * proj[1][0] + view[0][2] * proj[2][0] + view[0][3] * proj[3][0];
	clip[0][1] = view[0][0] * proj[0][1] + view[0][1] * proj[1][1] + view[0][2] * proj[2][1] + view[0][3] * proj[3][1];
	clip[0][2] = view[0][0] * proj[0][2] + view[0][1] * proj[1][2] + view[0][2] * proj[2][2] + view[0][3] * proj[3][2];
	clip[0][3] = view[0][0] * proj[0][3] + view[0][1] * proj[1][3] + view[0][2] * proj[2][3] + view[0][3] * proj[3][3];

	clip[1][0] = view[1][0] * proj[0][0] + view[1][1] * proj[1][0] + view[1][2] * proj[2][0] + view[1][3] * proj[3][0];
	clip[1][1] = view[1][0] * proj[0][1] + view[1][1] * proj[1][1] + view[1][2] * proj[2][1] + view[1][3] * proj[3][1];
	clip[1][2] = view[1][0] * proj[0][2] + view[1][1] * proj[1][2] + view[1][2] * proj[2][2] + view[1][3] * proj[3][2];
	clip[1][3] = view[1][0] * proj[0][3] + view[1][1] * proj[1][3] + view[1][2] * proj[2][3] + view[1][3] * proj[3][3];

	clip[2][0] = view[2][0] * proj[0][0] + view[2][1] * proj[1][0] + view[2][2] * proj[2][0] + view[2][3] * proj[3][0];
	clip[2][1] = view[2][0] * proj[0][1] + view[2][1] * proj[1][1] + view[2][2] * proj[2][1] + view[2][3] * proj[3][1];
	clip[2][2] = view[2][0] * proj[0][2] + view[2][1] * proj[1][2] + view[2][2] * proj[2][2] + view[2][3] * proj[3][2];
	clip[2][3] = view[2][0] * proj[0][3] + view[2][1] * proj[1][3] + view[2][2] * proj[2][3] + view[2][3] * proj[3][3];

	clip[3][0] = view[3][0] * proj[0][0] + view[3][1] * proj[1][0] + view[3][2] * proj[2][0] + view[3][3] * proj[3][0];
	clip[3][1] = view[3][0] * proj[0][1] + view[3][1] * proj[1][1] + view[3][2] * proj[2][1] + view[3][3] * proj[3][1];
	clip[3][2] = view[3][0] * proj[0][2] + view[3][1] * proj[1][2] + view[3][2] * proj[2][2] + view[3][3] * proj[3][2];
	clip[3][3] = view[3][0] * proj[0][3] + view[3][1] * proj[1][3] + view[3][2] * proj[2][3] + view[3][3] * proj[3][3];

	frustum_planes[Right][0] = clip[0][3] - clip[0][0];
	frustum_planes[Right][1] = clip[1][3] - clip[1][0];
	frustum_planes[Right][2] = clip[2][3] - clip[2][0];
	frustum_planes[Right][3] = clip[3][3] - clip[3][0];
	normalize_frustrum_plane(frustum_planes[Right]);

	frustum_planes[Left][0] = clip[0][3] + clip[0][0];
	frustum_planes[Left][1] = clip[1][3] + clip[1][0];
	frustum_planes[Left][2] = clip[2][3] + clip[2][0];
	frustum_planes[Left][3] = clip[3][3] + clip[3][0];
	normalize_frustrum_plane(frustum_planes[Left]);

	frustum_planes[Bottom][0] = clip[0][3] + clip[0][1];
	frustum_planes[Bottom][1] = clip[1][3] + clip[1][1];
	frustum_planes[Bottom][2] = clip[2][3] + clip[2][1];
	frustum_planes[Bottom][3] = clip[3][3] + clip[3][1];
	normalize_frustrum_plane(frustum_planes[Bottom]);

	frustum_planes[Top][0] = clip[0][3] - clip[0][1];
	frustum_planes[Top][1] = clip[1][3] - clip[1][1];
	frustum_planes[Top][2] = clip[2][3] - clip[2][1];
	frustum_planes[Top][3] = clip[3][3] - clip[3][1];
	normalize_frustrum_plane(frustum_planes[Top]);

	frustum_planes[Front][0] = clip[0][3] - clip[0][2];
	frustum_planes[Front][1] = clip[1][3] - clip[1][2];
	frustum_planes[Front][2] = clip[2][3] - clip[2][2];
	frustum_planes[Front][3] = clip[3][3] - clip[3][2];
	normalize_frustrum_plane(frustum_planes[Front]);

	frustum_planes[Back][0] = clip[0][3] + clip[0][2];
	frustum_planes[Back][1] = clip[1][3] + clip[1][2];
	frustum_planes[Back][2] = clip[2][3] + clip[2][2];
	frustum_planes[Back][3] = clip[3][3] + clip[3][2];
	normalize_frustrum_plane(frustum_planes[Back]);
}

void Camera::normalize_frustrum_plane(glm::vec4& plane) {
	float magnitude = glm::sqrt(
		plane[0] * plane[0] +
		plane[1] * plane[1] +
		plane[2] * plane[2]);

	plane[0] /= magnitude;
	plane[1] /= magnitude;
	plane[2] /= magnitude;
	plane[3] /= magnitude;
}


bool Camera::inside_frustrum(const glm::vec3& point) const {
	for (unsigned int i = 0; i < 6; i++) {
		if (frustum_planes[i][0] * point.x +
				frustum_planes[i][1] * point.y +
				frustum_planes[i][2] * point.z +
				frustum_planes[i][3] <=
			0) {
			return false;
		}
	}

	return true;
}

bool Camera::inside_frustrum(const glm::vec3& min, const glm::vec3& max) const {
	auto GetVisibility = [](const glm::vec4& clip, const glm::vec3& min, const glm::vec3& max) {
		float x0 = min.x * clip.x;
		float x1 = max.x * clip.x;
		float y0 = min.y * clip.y;
		float y1 = max.y * clip.y;
		float z0 = min.z * clip.z + clip.w;
		float z1 = max.z * clip.z + clip.w;
		float p1 = x0 + y0 + z0;
		float p2 = x1 + y0 + z0;
		float p3 = x1 + y1 + z0;
		float p4 = x0 + y1 + z0;
		float p5 = x0 + y0 + z1;
		float p6 = x1 + y0 + z1;
		float p7 = x1 + y1 + z1;
		float p8 = x0 + y1 + z1;

		return !(p1 <= 0.f && p2 <= 0.f && p3 <= 0.f && p4 <= 0.f && p5 <= 0.f && p6 <= 0.f && p7 <= 0.f && p8 <= 0.f);
	};

	bool v0 = GetVisibility(frustum_planes[Right], min, max);
	bool v1 = GetVisibility(frustum_planes[Left], min, max);
	bool v2 = GetVisibility(frustum_planes[Bottom], min, max);
	bool v3 = GetVisibility(frustum_planes[Top], min, max);
	bool v4 = GetVisibility(frustum_planes[Front], min, max);
	return v0 && v1 && v2 && v3 && v4;
}

void FPSCamera::update(const double delta) {
	float speed = 5;
	if (input_handler.key_pressed(Qt::Key_Shift)) {
		speed = 20;
	}

	if (input_handler.key_pressed(Qt::Key_W)) {
		position += direction * speed * static_cast<float>(delta);
	} else if (input_handler.key_pressed(Qt::Key_S)) {
		position -= direction * speed * static_cast<float>(delta);
	}

	const glm::vec3 displacement = glm::normalize(glm::cross(direction, up)) * speed * float(delta);
	if (input_handler.key_pressed(Qt::Key_A)) {
		position -= displacement;
	} else if (input_handler.key_pressed(Qt::Key_D)) {
		position += displacement;
	}

	if (input_handler.key_pressed(Qt::Key_Space)) {
		position.z += 1.f * speed * static_cast<float>(delta);
	} else if (input_handler.key_pressed(Qt::Key_Control)) {
		position.z -= 1.f * speed * static_cast<float>(delta);
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
	extract_frustrum_planes(projection, view);
}

void FPSCamera::mouse_move_event(QMouseEvent* event) {
	glm::vec2 diff = input_handler.mouse - input_handler.previous_mouse;

	horizontal_angle += diff.x * 0.012;
	vertical_angle += -diff.y * 0.012;
	vertical_angle = std::max(-glm::pi<float>() / 2.f + 0.001f, std::min(vertical_angle, glm::pi<float>() / 2.f - 0.001f));

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

	extract_frustrum_planes(projection, view);
}

void TPSCamera::mouse_move_event(QMouseEvent* event) {
	glm::vec2 diff = input_handler.mouse - input_handler.previous_mouse;

	if (rolling || (event->buttons() == Qt::RightButton && event->modifiers() & Qt::ControlModifier)) {
		horizontal_angle += diff.x * 0.0025f;
		vertical_angle -= diff.y * 0.0025f;
		vertical_angle = std::max(-glm::pi<float>() / 2.f + 0.001f, std::min(vertical_angle, glm::pi<float>() / 2.f - 0.001f));
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
			break;
        default:
			break;
	}
}

void TPSCamera::mouse_release_event(QMouseEvent* event) {
	switch (event->button()) {
		case Qt::MiddleButton:
			rolling = false;
			break;
        default:
			break;
	}
}

Camera* camera;