#include "stdafx.h"

Minimap::Minimap(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	show();
}

Minimap::~Minimap() {
}
