#include "stdafx.h"

bool InputHandler::key_pressed(Qt::Key key) const {
	return keys_pressed.count(key);
}

void InputHandler::mouse_move_event(QMouseEvent* event) {
	previous_mouse = mouse;
	mouse = event->globalPos();
}

InputHandler input_handler; 