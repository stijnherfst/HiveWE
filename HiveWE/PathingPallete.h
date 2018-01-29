#pragma once

#include "ui_PathingPallete.h"

class PathingPallete : public QWidget {
	Q_OBJECT

public:
	PathingPallete(QWidget *parent = Q_NULLPTR);
	~PathingPallete();

private:
	Ui::PathingPallete ui;
};