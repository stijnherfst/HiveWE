#pragma once

#include <unordered_set>

#include <QEvent>
#include <QMouseEvent>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

class InputHandler {
public:
	glm::vec2 mouse;
	glm::vec2 previous_mouse;

	glm::vec3 mouse_world;

	glm::vec3 drag_start;

	std::unordered_set<int> keys_pressed;

	void mouse_move_event(QMouseEvent* event);

	bool key_pressed(Qt::Key key) const;
};

extern InputHandler input_handler;