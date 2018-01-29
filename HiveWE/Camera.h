#pragma once

struct Camera {
	glm::vec3 position = { 0, 0, 5 };
	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };

	double fov = 45;
	double aspect_ratio = 16.0 / 9.0;

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

	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
	void mouse_scroll_event(QWheelEvent* event) override;
};

extern TPSCamera camera;