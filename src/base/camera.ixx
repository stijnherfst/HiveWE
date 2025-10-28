module;

#include <QWheelEvent>

export module Camera;

import std;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

export class InputHandler {
  public:
	glm::vec2 mouse;
	glm::vec2 previous_mouse;

	glm::vec3 mouse_world;
	glm::vec3 previous_mouse_world;

	glm::vec3 drag_start;

	std::unordered_set<int> keys_pressed;

	bool key_pressed(const Qt::Key key) const {
		return keys_pressed.contains(key);
	}

	void mouse_move_event(const QMouseEvent* event) {
		previous_mouse = mouse;
		mouse = {event->pos().x(), event->pos().y()};
	}
};

export inline InputHandler input_handler;

export struct Camera {
	glm::vec3 position = {0, 0, 0};

	float distance = 20.f;

	glm::vec3 direction = {0, 1, 0};
	glm::vec3 X = {1, 0, 0};
	glm::vec3 Y = {0, 1, 0};
	glm::vec3 up = {0, 0, 1};
	glm::vec3 forward = {0, 1, 0};

	float fov = 70.f;
	float aspect_ratio = 16.f / 9.f;
	float draw_distance_close = 0.05f;
	float draw_distance_far = 2000.f;
	float fov_rad = (glm::pi<double>() / 180.f) * static_cast<double>(fov); // Need radians
	float tan_height = 2.f * glm::tan(fov_rad * 0.5f);

	glm::mat4 projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance_far);
	glm::mat4 view = glm::lookAt(position - direction * distance, position, up);
	glm::mat4 view_inverse;
	glm::mat4 projection_view;

	float horizontal_angle = 0.f;
	float vertical_angle = -0.977f;

	struct Plane {
		glm::vec3 normal;
		float d;
	};

	Plane frustrum_planes[6];

	void extract_frustrum_planes() {
		const auto vp = projection_view;

		// Left
		frustrum_planes[0].normal = glm::vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]);
		frustrum_planes[0].d = vp[3][3] + vp[3][0];

		// Right
		frustrum_planes[1].normal = glm::vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]);
		frustrum_planes[1].d = vp[3][3] - vp[3][0];

		// Bottom
		frustrum_planes[2].normal = glm::vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]);
		frustrum_planes[2].d = vp[3][3] + vp[3][1];

		// Top
		frustrum_planes[3].normal = glm::vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]);
		frustrum_planes[3].d = vp[3][3] - vp[3][1];

		// Near
		frustrum_planes[4].normal = glm::vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]);
		frustrum_planes[4].d = vp[3][3] + vp[3][2];

		// Far
		frustrum_planes[5].normal = glm::vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]);
		frustrum_planes[5].d = vp[3][3] - vp[3][2];

		for (auto& p : frustrum_planes) {
			const float inv_len = 1.0f / glm::length(p.normal);
			p.normal *= inv_len;
			p.d *= inv_len;
		}
	}

	bool inside_frustrum(const glm::vec3& min, const glm::vec3& max) const {
		for (const auto& p : frustrum_planes) {
			glm::vec3 positive = min;
			if (p.normal.x >= 0) {
				positive.x = max.x;
			}
			if (p.normal.y >= 0) {
				positive.y = max.y;
			}
			if (p.normal.z >= 0) {
				positive.z = max.z;
			}

			if (glm::dot(p.normal, positive) + p.d < 0) {
				return false;
			}
		}
		return true;
	}

	bool inside_frustrum_transform(const glm::vec3& min, const glm::vec3& max, const glm::mat4& matrix) const {
		const glm::vec3 center = (min + max) * 0.5f;
		const glm::vec3 extents = (max - min) * 0.5f;

		const glm::vec3 new_center = glm::vec3(matrix * glm::vec4(center, 1.0f));

		const glm::mat3 rot_scale = glm::mat3(matrix);
		const glm::mat3 absolute_rotation_scale = glm::mat3(glm::abs(rot_scale[0]), glm::abs(rot_scale[1]), glm::abs(rot_scale[2]));

		const glm::vec3 new_extents = absolute_rotation_scale * extents;
		const glm::vec3 new_min = new_center - new_extents;
		const glm::vec3 new_max = new_center + new_extents;

		for (const auto& p : frustrum_planes) {
			glm::vec3 positive = new_min;
			if (p.normal.x >= 0) {
				positive.x = new_max.x;
			}
			if (p.normal.y >= 0) {
				positive.y = new_max.y;
			}
			if (p.normal.z >= 0) {
				positive.z = new_max.z;
			}

			if (glm::dot(p.normal, positive) + p.d < 0) {
				return false;
			}
		}
		return true;
	}

	void update(const double delta) {
		direction = glm::vec3(
			std::cos(vertical_angle) * std::sin(horizontal_angle),
			std::cos(vertical_angle) * std::cos(horizontal_angle),
			std::sin(vertical_angle)
		);

		direction = glm::normalize(direction);
		// Calculate axis directions for camera as referential point:
		X = glm::normalize(glm::cross(direction, up));
		Y = glm::normalize(glm::cross(X, direction));

		// The vector that is perpendicular to the up vector, thus points forward
		forward = glm::cross(X, up);

		if (input_handler.key_pressed(Qt::Key_Left)) {
			position += -X * 40.f * static_cast<float>(delta) * (distance / 30.f);
		} else if (input_handler.key_pressed(Qt::Key_Right)) {
			position += X * 40.f * static_cast<float>(delta) * (distance / 30.f);
		}

		if (input_handler.key_pressed(Qt::Key_Up)) {
			position += -forward * 40.f * static_cast<float>(delta) * (distance / 30.f);
		} else if (input_handler.key_pressed(Qt::Key_Down)) {
			position += forward * 40.f * static_cast<float>(delta) * (distance / 30.f);
		}

		projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance_far);
		view = glm::lookAt(position - direction * distance, position, up);
		view_inverse = glm::inverse(view);
		projection_view = projection * view;

		extract_frustrum_planes();
	}

	void mouse_move_event(QMouseEvent* event) {
		glm::vec2 diff = input_handler.mouse - input_handler.previous_mouse;

		if (event->buttons() == Qt::RightButton && event->modifiers() & Qt::ControlModifier) {
			horizontal_angle += diff.x * 0.0025f;
			vertical_angle -= diff.y * 0.0025f;
			vertical_angle = std::max(-glm::pi<float>() / 2.f + 0.001f, std::min(vertical_angle, glm::pi<float>() / 2.f - 0.001f));
			update(0.0);
		} else if (event->buttons() == Qt::RightButton) {
			position += X * (-diff.x * 0.025f * (distance / 30.f));
			position += forward * (-diff.y * 0.025f * (distance / 30.f));
			update(0.0);
		}
	}

	void mouse_scroll_event(const QWheelEvent* event) {
		distance = std::clamp(distance * std::pow(0.999f, static_cast<float>(event->angleDelta().y())), 0.001f, 1000.f);
		update(0.0);
	}

	void mouse_press_event(QMouseEvent* event) {}

	void mouse_release_event(QMouseEvent* event) {}

	void reset() {
		distance = 20.f;
		horizontal_angle = 0.0f;
		vertical_angle = -0.977f;
		update(0.0);
	}
};

export inline Camera camera;
