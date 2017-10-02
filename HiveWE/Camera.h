#pragma once

struct cameraStruct {
	double x = 10;
	double y = 0;
	double z = 20;
	double fov = 45;
	double aspectRatio = 16.0 / 9.0;
	glm::mat4 projection = glm::perspective(fov, aspectRatio, 0.1, 100.0);
	glm::mat4 view = glm::lookAt(glm::vec3(x, y - 3, z), glm::vec3(x, y, 0), glm::vec3(0, 0, 1));

	void update() {
		projection = glm::perspective(fov, aspectRatio, 0.1, 100.0);
		view = glm::lookAt(glm::vec3(x, y - 3, z), glm::vec3(x, y, 0), glm::vec3(0, 0, 1));
	}
};

extern cameraStruct camera;