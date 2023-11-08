#pragma once

#include "ui_unit_palette.h"
#include "unit_brush.h"
#include "palette.h"
//#include "UnitSelector.h"

#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QShortcut>
#include <QFrame>
#include <QGridLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMap>
#include <QScrollArea>
#include <QPushButton>
#include <QKeySequence>

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

	void select_id_in_palette(std::string id);

	Ui::UnitPalette ui;

	UnitBrush brush;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

	UnitSelector* selector;
	
	QRibbonSection* current_selection_section = new QRibbonSection;
	QLabel* selection_name = new QLabel;

	QShortcut* find_this;
	QShortcut* find_parent;

public slots:
	void deactivate(QRibbonTab* tab) override;
	void update_selection_info();
};