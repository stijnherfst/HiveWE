module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <unordered_set>

export module Camera;

export class InputHandler {
  public:
	glm::vec2 mouse;
	glm::vec2 previous_mouse;

	glm::vec3 mouse_world;
	glm::vec3 previous_mouse_world;

	glm::vec3 drag_start;

	std::unordered_set<int> keys_pressed;

	bool key_pressed(const Qt::Key key) const {
		return keys_pressed.count(key);
	}

	void mouse_move_event(QMouseEvent* event) {
		previous_mouse = mouse;
		mouse = { event->pos().x(), event->pos().y() };
	}
};

export inline InputHandler input_handler;

export struct Camera {
	glm::vec3 position = { 0, 0, 0 };

	float distance = 20.f;

	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 X = { 1, 0, 0 };
	glm::vec3 Y = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };
	glm::vec3 forward = { 0, 1, 0 };

	float fov = 70.f;
	float aspect_ratio = 16.f / 9.f;
	float draw_distance_close = 0.05f;
	float draw_distance_far = 2000.f;
	float fov_rad = (glm::pi<double>() / 180.f) * static_cast<double>(fov); // Need radians
	float tan_height = 2.f * glm::tan(fov_rad * 0.5f);

	glm::mat4 projection = glm::perspective(fov, aspect_ratio, draw_distance_close, draw_distance_far);
	glm::mat4 view = glm::lookAt(position - direction * distance, position, up);
	glm::mat4 projection_view;

	glm::vec4 frustum_planes[6];

	float horizontal_angle = 0.f;
	float vertical_angle = -0.977f;

	enum FrustrumPlane {
		Right,
		Left,
		Bottom,
		Top,
		Front,
		Back
	};

	void extract_frustrum_planes(const glm::mat4& proj, const glm::mat4& view) {
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

	void normalize_frustrum_plane(glm::vec4& plane) {
		float magnitude = glm::sqrt(
			plane[0] * plane[0] +
			plane[1] * plane[1] +
			plane[2] * plane[2]);

		plane[0] /= magnitude;
		plane[1] /= magnitude;
		plane[2] /= magnitude;
		plane[3] /= magnitude;
	}

	bool inside_frustrum(const glm::vec3& point) const {
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

	bool inside_frustrum(const glm::vec3& min, const glm::vec3& max) const {
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

	void update(double delta) {
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
		projection_view = projection * view;

		extract_frustrum_planes(projection, view);
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

	void mouse_scroll_event(QWheelEvent* event) {
		distance = std::clamp(distance * std::pow(0.999f, static_cast<float>(event->angleDelta().y())), 0.001f, 1000.f);
		update(0.0);
	}

	void mouse_press_event(QMouseEvent* event) {	
	}

	void mouse_release_event(QMouseEvent* event) {
	}

	void reset() {
		distance = 20.f;
		horizontal_angle = 0.0f;
		vertical_angle = -0.977f;
		update(0.0);
	}
};

export inline Camera camera;