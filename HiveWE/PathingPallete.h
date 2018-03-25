#pragma once

#include "ui_PathingPallete.h"

class PathingPallete : public QWidget {
	Q_OBJECT

public:
	explicit PathingPallete(QWidget *parent = Q_NULLPTR);
	~PathingPallete();

private:
	bool event(QEvent *e) override;

	Ui::PathingPallete ui;
	PathingBrush brush;
};