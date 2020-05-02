#pragma once

#include <QDialog>

#include "ui_PathingPalette.h"
#include "PathingBrush.h"

class PathingPalette : public QDialog {
	Q_OBJECT

public:
	explicit PathingPalette(QWidget* parent = nullptr);
	~PathingPalette();

private:
	bool event(QEvent *e) override;

	Ui::PathingPalette ui;
	PathingBrush brush;
};