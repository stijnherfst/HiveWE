#pragma once

#include "ui_UnitPalette.h"
#include "UnitBrush.h"
#include "Palette.h"
#include "QRibbon.h"
#include "UnitListModel.h"
//#include "UnitSelector.h"

class UnitPalette : public Palette {
	Q_OBJECT

public:
	UnitPalette(QWidget* parent = nullptr);
	~UnitPalette();

private:
	bool event(QEvent* e) override;

	Ui::UnitPalette ui;

	UnitBrush brush;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

	QShortcut* find_this;
	QShortcut* find_parent;

public slots:
	void deactivate(QRibbonTab* tab) override;
};