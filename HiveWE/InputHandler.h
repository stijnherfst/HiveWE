#pragma once

class InputHandler {
public:
	QPoint mouse;
	QPoint previous_mouse;

	glm::vec3 mouse_world;

	glm::vec3 drag_start;

	std::unordered_set<int> keys_pressed;

	void mouse_move_event(QMouseEvent* event);

	bool key_pressed(Qt::Key key);
};

extern InputHandler input_handler;