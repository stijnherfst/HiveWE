#pragma once

#include <QWidget>
#include "ui_TerrainPalette.h"

class TerrainPalette : public QWidget {
	Q_OBJECT

public:
	TerrainPalette(QWidget *parent = Q_NULLPTR);
	~TerrainPalette();

private:
	bool event(QEvent *e) override;

	Ui::TerrainPalette ui;
	QButtonGroup* textures_group = new QButtonGroup;
	FlowLayout* textures_layout = new FlowLayout;

	TerrainBrush brush;
};
