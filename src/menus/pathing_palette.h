#pragma once

#include <QDialog>

#include "ui_pathing_palette.h"

#include "palette.h"
#include "pathing_brush.h"

class PathingPalette : public Palette {
	Q_OBJECT

public:
	explicit PathingPalette(QWidget* parent = nullptr);
	~PathingPalette();

private:
	bool event(QEvent *e) override;

	Ui::PathingPalette ui;
	PathingBrush brush;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;
	QRibbonButton* import_pathing = new QRibbonButton;
	QRibbonButton* export_pathing = new QRibbonButton;

public slots:
	void deactivate(QRibbonTab* tab) override;
};