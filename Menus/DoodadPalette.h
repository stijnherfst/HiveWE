#pragma once

#include "ui_DoodadPalette.h"
#include "DoodadBrush.h"
#include "Palette.h"
#include "QRibbon.h"
#include "AspectRatioPixmapLabel.h"
#include "DoodadListModel.h"
#include "DestructableListModel.h"


class DoodadPalette : public Palette {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void selection_changed(const QModelIndex& index);

	Ui::DoodadPalette ui;

	DoodadBrush brush;
	DoodadListModel* doodad_list_model;
	DoodadListFilter* doodad_filter_model;
	DestructableListModel* destructable_list_model;
	DestructableListFilter* destructable_filter_model;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;
	QRibbonButton* selections_button = new QRibbonButton;

	QRibbonContainer* variations = new QRibbonContainer;

	QRibbonSection* pathing_section = new QRibbonSection;
	AspectRatioPixmapLabel* pathing_image_label = new AspectRatioPixmapLabel;

public slots:
	void deactivate(QRibbonTab* tab) override;
};