#include "stdafx.h"

PathingPallete::PathingPallete(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);

	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

	show();

	connect(ui.replaceType, &QPushButton::clicked, [&]() { map.brush.operation = PathingBrush::Operation::replace; });
	connect(ui.addType, &QPushButton::clicked, [&]() { map.brush.operation = PathingBrush::Operation::add; });
	connect(ui.removeType, &QPushButton::clicked, [&]() { map.brush.operation = PathingBrush::Operation::remove; });

	connect(ui.brushTypeGroup, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [&]() {
		map.brush.brush_mask = 0;

		if (ui.walkable->isChecked()) {
			map.brush.brush_mask |= 0b00000010;
		}
		if (ui.flyable->isChecked()) {
			map.brush.brush_mask |= 0b00000100;
		}
		if (ui.buildable->isChecked()) {
			map.brush.brush_mask |= 0b00001000;
		}
	});

	connect(ui.brushSizeGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) { 
		map.brush.set_size(button->text().toInt() - 1); 
		ui.brushSize->setValue(button->text().toInt());
	});

	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) { map.brush.set_size(value - 1); });

}

PathingPallete::~PathingPallete() {
	deleteLater();
}