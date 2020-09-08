#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QEvent>
#include <QMouseEvent>

struct Camera {
	virtual ~Camera() = default;

	glm::vec3 position = { 0, 0, 0 };

	float distance = 20.f;

	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 X = { 1, 0, 0 };
	glm::vec3 Y = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };
	glm::vec3 forward = { 0, 1, 0 };

	float fov = 70.f;
	float aspect_ratio = 16.f / 9.f;
	float draw_distance = 2000.f;
	float draw_distance_close = 0.05f;
	float fov_rad = (glm::pi<double>() / 180.f) * static_cast<double>(fov); // Need radians
	float tan_height = 2.f * glm::tan(fov_rad * 0.5f);

	glm::mat4 projection = glm::perspective(fov, aspect_ratio, draw_distance, draw_distance_close);
	glm::mat4 view = glm::lookAt(position - direction * distance, position, up);
	glm::mat4 projection_view;

	glm::vec4 frustum_planes[6];

	// Used for decomposing camera information to get rotation, for camera-centric Billboarded model elements
	glm::vec3 decomposed_scale;
	glm::quat decomposed_rotation;
	glm::vec3 decomposed_translation;
	glm::vec3 decomposed_skew;
	glm::vec4 decomposed_perspective;

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

	void extract_frustrum_planes(const glm::mat4& proj, const glm::mat4& view);
	void normalize_frustrum_plane(glm::vec4& plane);
	bool inside_frustrum(const glm::vec3& point) const;
	bool inside_frustrum(const glm::vec3& min, const glm::vec3& max) const;

	virtual void update(double delta) = 0;

	virtual void mouse_move_event(QMouseEvent* event) = 0;
	virtual void mouse_scroll_event(QWheelEvent* event) = 0;
	virtual void mouse_press_event(QMouseEvent* event) = 0;
	virtual void mouse_release_event(QMouseEvent* event) = 0;
	virtual void reset();
};

struct FPSCamera : Camera {
	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
	void mouse_scroll_event(QWheelEvent* event) override;
	void mouse_press_event(QMouseEvent* event) override{};
	void mouse_release_event(QMouseEvent* event) override{};
};

struct TPSCamera : Camera {

	bool rolling = false;

	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
	void mouse_scroll_event(QWheelEvent* event) override;
	void mouse_press_event(QMouseEvent* event) override;
	void mouse_release_event(QMouseEvent* event) override;
};

extern Camera* camera;