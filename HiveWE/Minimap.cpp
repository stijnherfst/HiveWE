#include "stdafx.h"

Minimap::Minimap(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	show();
}

Minimap::~Minimap() {
}

void Minimap::set_minimap(Texture texture) {
	QImage temp_image = QImage(texture.data.data(), texture.width, texture.height, texture.width * texture.channels, QImage::Format::Format_RGBA8888);
	ui.image->setPixmap(QPixmap::fromImage(temp_image.scaled(256, 256)));
}