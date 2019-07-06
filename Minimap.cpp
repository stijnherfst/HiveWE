#include "stdafx.h"

Minimap::Minimap(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	show();
}

void Minimap::set_minimap(Texture texture) {
	QImage temp_image = QImage(texture.data.data(), texture.width, texture.height, texture.width * texture.channels, QImage::Format::Format_RGBA8888);
	ui.image->setPixmap(QPixmap::fromImage(temp_image));
}

int x_offset;
int y_offset;

void Minimap::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			x_offset = event->globalPos().x() - geometry().x();
			y_offset = event->globalPos().y() - geometry().y();
		} else {
			float x = (event->localPos().x() - ui.image->horizontal_border) / (width() - ui.image->horizontal_border * 2.f);
			float y = (event->localPos().y() - ui.image->vertical_border) / (height() - ui.image->vertical_border * 2.f);

			x = std::clamp(x, 0.f, 1.f);
			y = std::clamp(y, 0.f, 1.f);

			emit clicked({ x, y });
		}
	}
}

void Minimap::mouseReleaseEvent(QMouseEvent*) {
	setCursor(Qt::ArrowCursor);
}

void Minimap::mouseMoveEvent(QMouseEvent* event) {
	if (event->modifiers() & Qt::ControlModifier) {
		setCursor(Qt::SizeAllCursor);
	}
	if (event->buttons() & Qt::LeftButton) {
		if (event->modifiers() & Qt::ControlModifier) {
			move(event->globalPos().x() - x_offset, event->globalPos().y() - y_offset);
		} else if (rect().contains(event->localPos().toPoint())) {
			float x = (event->localPos().x() - ui.image->horizontal_border) / (width() - ui.image->horizontal_border * 2.f);
			float y = (event->localPos().y() - ui.image->vertical_border) / (height() - ui.image->vertical_border * 2.f);
			
			x = std::clamp(x, 0.f, 1.f);
			y = std::clamp(y, 0.f, 1.f);

			emit clicked({ x, y });
		}
	}
}