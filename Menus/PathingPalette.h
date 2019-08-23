#pragma once

#include <QDialog>

#include "ui_PathingPalette.h"
#include "PathingBrush.h"

class PathingPallete : public QDialog {
	Q_OBJECT

public:
	explicit PathingPallete(QWidget* parent = nullptr);
	~PathingPallete();

private:
	bool event(QEvent *e) override;

	Ui::PathingPallete ui;
	PathingBrush brush;
};