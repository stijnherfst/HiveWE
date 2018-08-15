#pragma once

#include <QWidget>
#include "ui_TerrainPalette.h"

class TerrainPalette : public QDialog {
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
};