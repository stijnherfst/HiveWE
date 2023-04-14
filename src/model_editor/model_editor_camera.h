#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QEvent>
#include <QMouseEvent>

import Camera;

struct ModelEditorCamera {
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

	// Used for decomposing camera information to get rotation, for camera-centric Billboarded model elements
	glm::vec3 decomposed_scale;
	glm::quat decomposed_rotation;
	glm::vec3 decomposed_translation;
	glm::vec3 decomposed_skew;
	glm::vec4 decomposed_perspective;

	float horizontal_angle = 0.f;
	float vertical_angle = -0.977f;

	bool rolling = false;

	void update(double delta);

	void mouse_move_event(QMouseEvent* event, const InputHandler& my_input_handler);
	void mouse_scroll_event(QWheelEvent* event);
	void mouse_press_event(QMouseEvent* event);
	void mouse_release_event(QMouseEvent* event);
	void reset();
};