#include "InputHandler.h"


bool InputHandler::key_pressed(const Qt::Key key) const {
	return keys_pressed.count(key);
}

void InputHandler::mouse_move_event(QMouseEvent* event) {
	previous_mouse = mouse;
	mouse = event->pos();
}

InputHandler input_handler; 