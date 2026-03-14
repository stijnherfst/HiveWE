#pragma once

#include "ui_terrain_palette.h"

#include <QDialog>

#include "palette.h"
#include "terrain_brush.h"

import QRibbon;
import FlowLayout;

class TerrainPalette: public Palette {
	Q_OBJECT

  public:
	TerrainPalette(QWidget* parent = nullptr);
	~TerrainPalette();

  private:
	bool event(QEvent* e) override;

	Ui::TerrainPalette ui;

	TerrainBrush brush;

	QButtonGroup* textures_group = new QButtonGroup;
	FlowLayout* textures_layout = new FlowLayout;

	QButtonGroup* cliff_group = new QButtonGroup;
	FlowLayout* cliff_layout = new FlowLayout;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;

	QShortcut* change_mode_this;
	QShortcut* change_mode_parent;

	// GUI creation functions
	void update_operator_gui();

	// textures
	void update_texture_operator_gui();
	void setup_texture_operator();
	void create_terrain_buttons();
	QPushButton* terrain_button(const QIcon& icon, const char* propertyName, const QVariant& propertyValue, const QString& tileName);

	// cliff
	void update_cliff_operator_gui();
	void setup_cliff_operator();

	// deformation
	void update_deformation_operator_gui();
	void setup_deformation_operator();

	// cell operator (boundaries, water, holes)
	void update_cell_operator_gui();
	void setup_cell_operator();

	// ribbon
	void create_ribbon();

	// brush menu
	void setup_brush_menu();

  public slots:
	void deactivate(QRibbonTab* tab) override;
};
