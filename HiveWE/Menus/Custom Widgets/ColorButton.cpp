#include "stdafx.h"

ColorButton::ColorButton(QWidget* parent) : QPushButton(parent) {
	connect(this, &QPushButton::clicked, this, &ColorButton::changeColor);
}

void ColorButton::changeColor() {
	QColor newColor = QColorDialog::getColor(color, parentWidget());
	if (newColor != color) {
		setColor(newColor);
	}
}

void ColorButton::setColor(const QColor& new_color) {
	color = new_color;

	int delta = color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114;
	QColor text_color = QColor((255 - delta < 105) ? Qt::black : Qt::white);

	setStyleSheet("border-color: " + color.name() + "; " + QString("background-color: ") + color.name() + "; color: " + text_color.name());
}

const QColor& ColorButton::getColor() {
	return color;
}

glm::vec4 ColorButton::get_glm_color() const {
	return { color.red(), color.green(), color.blue(), color.alpha() };
}