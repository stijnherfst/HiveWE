#pragma once

#include "ui_TerrainPalette.h"

#include <QDialog>
#include "FlowLayout.h"

#include "Palette.h"
#include "TerrainBrush.h"
#include "QRibbon.h"

class TerrainPalette : public Palette {
	Q_OBJECT

public:
	TerrainPalette(QWidget* parent = nullptr);
	~TerrainPalette();

private:
	bool event(QEvent *e) override;

	Ui::TerrainPalette ui;
	QButtonGroup* textures_group = new QButtonGroup;
	FlowLayout* textures_layout = new FlowLayout;

	QButtonGroup* cliff_group = new QButtonGroup;
	FlowLayout* cliff_layout = new FlowLayout;

	TerrainBrush brush;
	QRibbonTab* ribbon_tab = new QRibbonTab;

public slots:
	void deactivate(QRibbonTab* tab) override;
};