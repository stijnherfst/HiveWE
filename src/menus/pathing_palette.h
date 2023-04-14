#pragma once

#include <QDialog>

#include "ui_PathingPalette.h"
#include "PathingBrush.h"

#include "Palette.h"

class PathingPalette : public Palette {
	Q_OBJECT

public:
	explicit PathingPalette(QWidget* parent = nullptr);
	~PathingPalette();

private:
	bool event(QEvent *e) override;

	Ui::PathingPalette ui;
	PathingBrush brush;

public slots:
	void deactivate(QRibbonTab* tab) override;
};