#pragma once

struct Camera {
	glm::vec3 position = { 0, 0, 5 };
	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };

	double fov = 45;
	double aspectRatio = 16.0 / 9.0;

	glm::mat4 projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
	glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0, 0, 1));
	glm::mat4 projection_view;

	double horizontalAngle = 0.0;
	double verticalAngle = 0.0;

	virtual void update(double delta) = 0;

	virtual void mouse_move_event(QMouseEvent* event) = 0;
};

struct FPSCamera : Camera {
	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
};

struct TPSCamera : Camera {
	float distance = 15;


	void update(double delta) override;
	void mouse_move_event(QMouseEvent* event) override;
};


struct cameraStruct {
	glm::vec3 position = { 0, 0, 5 };
	glm::vec3 direction = { 0, 1, 0 };
	glm::vec3 up = { 0, 0, 1 };

	double fov = 45;
	double aspectRatio = 16.0 / 9.0;

	glm::mat4 projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
	glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0, 0, 1));
	glm::mat4 projection_view;

	double horizontalAngle = 0.0;
	double verticalAngle = 0.0;

	void update() {
		verticalAngle = std::max(-89.0, std::min(verticalAngle, 89.0));

		direction = glm::vec3 (
			std::cos(verticalAngle) * std::sin(horizontalAngle),
			std::cos(verticalAngle) * std::cos(horizontalAngle),
			sin(verticalAngle)
		);

		direction = glm::normalize(direction);

		projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
		view = glm::lookAt(position, position + direction, up);
		projection_view = projection * view;
	}
};

extern TPSCamera camera;