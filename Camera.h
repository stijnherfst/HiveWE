#pragma once

struct Camera {
	virtual ~Camera() = default;

	glm::vec3 position = { 0, 0, 0 };


	float distance = 20;

	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 X = { 1, 0, 0 };
	glm::vec3 Y = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };
	glm::vec3 forward = { 0, 1, 0 };

	double fov = 70;
	double aspect_ratio = 16.0 / 9.0;
	double draw_distance = 2000.0;
	double draw_distance_close = 0.05;
	double fov_rad = (glm::pi<double>() / 180.0) * static_cast<double>(fov); // Need radians
	double tan_height = 2.0 * glm::tan(fov_rad * 0.5);

	glm::mat4 projection = glm::perspective(fov, aspect_ratio, draw_distance, draw_distance_close);
	glm::mat4 view = glm::lookAt(position - direction * distance, position, up);
	glm::mat4 projection_view;

	double horizontal_angle = 0.0;
	double vertical_angle = -0.977;

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
	void mouse_press_event(QMouseEvent* event) override {};
	void mouse_release_event(QMouseEvent* event) override {};
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