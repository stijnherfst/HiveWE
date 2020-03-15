#pragma once

//#include "ui_UnitPalette.h"
#include "UnitBrush.h"
#include "Palette.h"
#include "QRibbon.h"
#include "AspectRatioPixmapLabel.h"

class UnitPalette : public Palette {
	Q_OBJECT

public:
	UnitPalette(QWidget* parent = nullptr);
	~UnitPalette();

private:
	bool event(QEvent* e) override;

	void update_list();

	//Ui::DoodadPalette ui;

	UnitBrush brush;

public slots:
	void deactivate(QRibbonTab* tab) override;
};