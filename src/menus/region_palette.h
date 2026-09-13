#pragma once

#include "ui_region_palette.h"

#include "palette.h"
#include "region_brush.h"

import QRibbon;
import Regions;
import Sounds;
import WorldUndoManager;

class RegionPalette : public Palette {
	Q_OBJECT

public:
	RegionPalette(QWidget* parent, Brush*& active_brush, Regions& regions, Sounds& sounds, WorldUndoManager& world_undo);
	~RegionPalette();

private:
	bool event(QEvent* e) override;

	/// Rebuilds the region list from the map regions and syncs the list selection with the brush selection
	void update_list();
	void update_properties();

	Regions& regions;
	Sounds& sounds;

	Ui::RegionPalette ui;
	RegionBrush brush;

	/// Guards against the list/property widgets and the brush updating each other in a loop
	bool updating = false;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

public slots:
	void deactivate(QRibbonTab* tab) override;
};
