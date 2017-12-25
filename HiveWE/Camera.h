#pragma once

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
			cos(verticalAngle) * sin(horizontalAngle),
			cos(verticalAngle) * cos(horizontalAngle),
			sin(verticalAngle)
		);

		direction = glm::normalize(direction);

		projection = glm::perspective(fov, aspectRatio, 0.1, 1000.0);
		view = glm::lookAt(position, position + direction, up);
		projection_view = projection * view;
	}
};

extern cameraStruct camera;