#include "stdafx.h"

Minimap::Minimap(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	show();
}

void Minimap::set_minimap(Texture texture) {
	QImage temp_image = QImage(texture.data.data(), texture.width, texture.height, texture.width * texture.channels, QImage::Format::Format_RGBA8888);
	ui.image->setPixmap(QPixmap::fromImage(temp_image));
}

void Minimap::mousePressEvent(QMouseEvent* event) {
	emit clicked({ event->localPos().x() / width(), event->localPos().y() / height() });
}

void Minimap::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() & Qt::LeftButton) {
		emit clicked({ event->localPos().x() / width(), event->localPos().y() / height() });
	}
}