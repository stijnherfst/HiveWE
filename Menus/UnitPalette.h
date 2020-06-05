#pragma once

#include "ui_UnitPalette.h"
#include "UnitBrush.h"
#include "Palette.h"
#include "QRibbon.h"
#include "UnitListModel.h"


class UnitPalette : public Palette {
	Q_OBJECT

public:
	UnitPalette(QWidget* parent = nullptr);
	~UnitPalette();

private:
	bool event(QEvent* e) override;

	void selection_changed(const QModelIndex& item);

	Ui::UnitPalette ui;

	UnitBrush brush;
	UnitListModel* list_model;
	UnitListFilter* filter_model;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

	//QShortcut* find = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
	QShortcut* find;

public slots:
	void deactivate(QRibbonTab* tab) override;
};