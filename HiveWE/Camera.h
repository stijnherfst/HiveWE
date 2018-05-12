#pragma once

struct Camera {
	virtual ~Camera() = default;

	glm::vec3 position = { 0, 0, 5 };
	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 X = { 1, 0, 0 };
	glm::vec3 Y = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };

	double fov = 45;
	double aspect_ratio = 16.0 / 9.0;
	float draw_distance = 200.0;
	float draw_distance_close = 0.1;
	float fov_rad = (glm::pi<float>() / 180.f) * static_cast<float>(fov); // Need radians
	float tan_height = 2.f * glm::tan(fov_rad * 0.5f);

	glm::mat4 projection = glm::perspective(fov, aspect_ratio, 0.1, 1000.0);
	glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0, 0, 1));
	glm::mat4 projection_view;

	double horizontal_angle = 0.0;
	double vertical_angle = 0.0;

	virtual void update(double delta) = 0;

	virtual void mouse_move_event(QMouseEvent* event) = 0;
	virtual void mouse_scroll_event(QWheelEvent* event) = 0;
};

struct FPSCamera : Camera {
	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
	void mouse_scroll_event(QWheelEvent* event) override;
};

struct TPSCamera : Camera {
	float distance = 15;
	double vertical_angle = 1;
	double horizontal_angle = 3.1415;

	bool rolling = false;

	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
	void mouse_scroll_event(QWheelEvent* event) override;
	void mouse_press_event(QMouseEvent* event);
	void mouse_release_event(QMouseEvent* event);
	bool is_visible(glm::vec3 &&point);
};

extern TPSCamera camera;