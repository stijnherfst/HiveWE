#include "stdafx.h"

PathingPallete::PathingPallete(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.create();
	map->brush = &brush;

	connect(ui.replaceType, &QPushButton::clicked, [&]() { brush.operation = PathingBrush::Operation::replace; });
	connect(ui.addType, &QPushButton::clicked, [&]() { brush.operation = PathingBrush::Operation::add; });
	connect(ui.removeType, &QPushButton::clicked, [&]() { brush.operation = PathingBrush::Operation::remove; });

	connect(ui.brushTypeGroup, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [&]() {
		brush.brush_mask = 0;

		if (ui.walkable->isChecked()) {
			brush.brush_mask |= 0b00000010;
		}
		if (ui.flyable->isChecked()) {
			brush.brush_mask |= 0b00000100;
		}
		if (ui.buildable->isChecked()) {
			brush.brush_mask |= 0b00001000;
		}
	});

	connect(ui.brushSizeGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) { 
		brush.set_size(button->text().toInt()); 
		ui.brushSize->setValue(button->text().toInt());
	});

	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) { brush.set_size(value); });

	connect(ui.brushShapeCircle, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::circle); });
	connect(ui.brushShapeSquare, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::square); });
	connect(ui.brushShapeDiamond, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::diamond); });
}

PathingPallete::~PathingPallete() {
	map->brush = nullptr;
}

bool PathingPallete::event(QEvent *e) {
	if (e->type() == QEvent::WindowActivate) {
		map->brush = &brush;
	}
	return QWidget::event(e);
}