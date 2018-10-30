#pragma once

#include "ui_DoodadPalette.h"

class DoodadPalette : public Palette {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void update_list();
	void selection_changed(QListWidgetItem* item);

	Ui::DoodadPalette ui;

	DoodadBrush brush;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

	QRibbonContainer* variations = new QRibbonContainer;

	QRibbonSection* pathing_section = new QRibbonSection;
	AspectRatioPixmapLabel* pathing_image_label = new AspectRatioPixmapLabel;

public slots:
	void deactivate(QRibbonTab* tab) override;
};