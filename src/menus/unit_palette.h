#pragma once

#include "ui_unit_palette.h"
#include "unit_brush.h"
#include "palette.h"
//#include "UnitSelector.h"

#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>

import UnitListModel;
import UnitSelector;
import QRibbon;

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

	UnitSelector* selector;

	QShortcut* find_this;
	QShortcut* find_parent;

public slots:
	void deactivate(QRibbonTab* tab) override;
};